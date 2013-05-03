/*
 Copyright (C) 2013 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Rand.h"
#include "cinder/Surface.h"
#include "cinder/Text.h"

#include "boost/assign.hpp"
#include "boost/polygon/segment_data.hpp"
#include "boost/polygon/voronoi.hpp"

#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class UVorApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void keyDown( KeyEvent event );

		void update();
		void draw();

		struct Segment2d
		{
			Segment2d( const Vec2d &a, const Vec2d &b ) : p0( a ), p1( b ) {}
			Vec2d p0, p1;
		};

	private:
		Surface8u mRendered;
		gl::Texture mTexture;

		typedef double CoordinateType;
		typedef boost::polygon::point_data< CoordinateType > PointType;
		typedef boost::polygon::segment_data< CoordinateType > SegmentType;
		typedef boost::polygon::voronoi_diagram< CoordinateType > VoronoiDiagram;
		typedef VoronoiDiagram::cell_type CellType;
		typedef VoronoiDiagram::cell_type::source_index_type SourceIndexType;
		typedef VoronoiDiagram::cell_type::source_category_type SourceCategoryType;
		typedef VoronoiDiagram::edge_type EdgeType;

		vector< Vec2d > mPoints;
		vector< Segment2d > mSegments;
		VoronoiDiagram mVd;

		Vec2d retrievePoint( const CellType &cell );
		Segment2d retrieveSegment( const CellType &cell );
		void clipInfiniteEdge( const EdgeType &edge, Vec2f *e0, Vec2f *e1 );
};

// register Vec2d and Segment2d with boost polygon
namespace boost { namespace polygon {
	template <>
	struct geometry_concept< ci::Vec2d > { typedef point_concept type; };

	template <>
	struct point_traits< ci::Vec2d >
	{
		typedef double coordinate_type;

		static inline coordinate_type get( const ci::Vec2d &point, orientation_2d orient )
		{
			return ( orient == HORIZONTAL ) ? point.x : point.y;
		}
	};

	template <>
	struct geometry_concept< UVorApp::Segment2d > { typedef segment_concept type; };

	template <>
	struct segment_traits< UVorApp::Segment2d >
	{
		typedef double coordinate_type;
		typedef ci::Vec2d point_type;

		static inline point_type get( const UVorApp::Segment2d &segment, direction_1d dir )
		{
			return dir.to_int() ? segment.p1 : segment.p0;
		}
	};
} } // namespace boost::polygon

void UVorApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1280, 800 );
}

void UVorApp::setup()
{
	TextLayout layout;
	layout.clear( Color::black() );
	layout.setFont( Font( loadResource( RES_CUSTOM_FONT ), 150 ) );
	layout.setLeadingOffset( 50 );
	layout.setColor( Color::white() );
	vector< vector< int > > strs;
	strs.push_back( boost::assign::list_of( 0xffffffc2 )( 0xffffffa1 )( 0xffffffcd )( 0xffffffa3 )( 0xffffffc8 )( 0xffffffa5 )( 0xffffffc2 )( 0xffffffa7 )( 0xffffffc7 )( 0xffffffa9 )( 0xffffffcd ) );
	strs.push_back( boost::assign::list_of( 0xffffffd2 )( 0xffffffa2 )( 0xffffffd9 )( 0xffffffa4 )( 0x46 )( 0x1a )( 0xffffffa7 )( 0xffffffc4 )( 0xffffffa9 )( 0xffffffcf )( 0xffffffab )( 0xffffffd8 )( 0xffffffad )( 0x4d )( 0x6 )( 0xffffffb0 )( 0xffffffc2 ) );
	strs.push_back( boost::assign::list_of( 0xffffffcc )( 0xffffffa3 )( 0xffffffc5 )( 0xffffffa5 )( 0xffffffd6 )( 0xffffffa7 )( 0xffffffc7 )( 0xffffffa9 )( 0xffffffde )( 0xffffffaa )( 0xffffffac ) );

	int i = 0;
	for ( auto it = strs.begin(); it != strs.end(); ++it )
	{
		string str;
		for ( int j = 0; j < it->size(); j++ )
		{
			str += it->at( j ) ^ ( i + j + 128 );
		}
		layout.addCenteredLine( str );
		i++;
	}
	mRendered = layout.render( true );
	mTexture = gl::Texture( mRendered );
}

void UVorApp::update()
{
	Vec2i off = ( getWindowSize() - mRendered.getSize() ) / 2;
	mPoints.clear();
	Rand::randSeed( Rand::randInt() );
	while ( mPoints.size() < 5000 )
	{
		Vec2i p( Rand::randInt( mRendered.getWidth() ), Rand::randInt( mRendered.getHeight() ) );
		if ( mRendered.getPixel( p ).r > 0 )
		{
			mPoints.push_back( p + off );
		}
	}

	mVd.clear();
	construct_voronoi( mPoints.begin(), mPoints.end(), &mVd );
}

Vec2d UVorApp::retrievePoint( const CellType &cell )
{
	SourceIndexType index = cell.source_index();
	SourceCategoryType category = cell.source_category();
	if ( category == boost::polygon::SOURCE_CATEGORY_SINGLE_POINT )
	{
		return mPoints[ index ];
	}

	index -= mPoints.size();
	if ( category == boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT )
	{
		return mSegments[ index ].p0;
	}
	else
	{
		return mSegments[ index ].p1;
	}
}

UVorApp::Segment2d UVorApp::retrieveSegment( const CellType &cell )
{
	SourceIndexType index = cell.source_index() - mPoints.size();
	return mSegments[ index ];
}

void UVorApp::clipInfiniteEdge( const EdgeType &edge, Vec2f *e0, Vec2f *e1 )
{
	const CellType &cell1 = *edge.cell();
	const CellType &cell2 = *edge.twin()->cell();

	Vec2d origin, direction;

	// Infinite edges could not be created by two segment sites.
	if ( cell1.contains_point() && cell2.contains_point() )
	{
		Vec2d p1 = retrievePoint( cell1 );
		Vec2d p2 = retrievePoint( cell2 );
		origin = ( p1 + p2 ) * 0.5;
		direction= Vec2d( p1.y - p2.y, p2.x - p1.x );
	}
	else
	{
		origin = cell1.contains_segment() ?
						retrievePoint( cell2 ) : retrievePoint( cell1 );
		Segment2d segment = cell1.contains_segment() ?
						retrieveSegment( cell1 ) : retrieveSegment( cell2 );
		Vec2d dseg = segment.p1 - segment.p0;
		if ( ( segment.p0 == origin ) ^ cell1.contains_point() )
		{
			direction.x = dseg.y;
			direction.y = -dseg.x;
		}
		else
		{
			direction.x = -dseg.y;
			direction.y = dseg.x;
		}
	}

	double side = getWindowWidth();
	double coef = side / math< double >::max(
			math< double >::abs( direction.x ),
			math< double >::abs( direction.y ) );
	if ( edge.vertex0() == NULL )
		*e0 = origin - direction * coef;
	else
		*e0 = Vec2f( edge.vertex0()->x(), edge.vertex0()->y() );

	if ( edge.vertex1() == NULL )
		*e1 = origin + direction * coef;
	else
		*e1 = Vec2f( edge.vertex1()->x(), edge.vertex1()->y() );
}

void UVorApp::draw()
{
	gl::clear();

	gl::color( Color::white() );
	gl::begin( GL_LINES );
	for ( auto it = mVd.edges().begin(); it != mVd.edges().end(); ++it )
	{
		auto edge = *it;

		if ( edge.is_secondary() )
			continue;

		if ( edge.is_linear() )
		{
			auto *v0 = edge.vertex0();
			auto *v1 = edge.vertex1();

			Vec2f p0, p1;
			if ( edge.is_infinite() )
			{
				clipInfiniteEdge( edge, &p0, &p1 );
			}
			else
			{
				p0 = Vec2f( v0->x(), v0->y() );
				p1 = Vec2f( v1->x(), v1->y() );
			}

			Vec2d point = edge.cell()->contains_point() ?
				retrievePoint( *edge.cell() ) :
				retrievePoint( *edge.twin()->cell() );
			const double dMax = 16.;
			double d0 = math< double >::clamp( point.distance( p0 ), 0, dMax );
			double d1 = math< double >::clamp( point.distance( p1 ), 0, dMax );
			gl::color( Color::gray( 1. - d0 / dMax ) );
			gl::vertex( p0 );
			gl::color( Color::gray( 1. - d1 / dMax ) );
			gl::vertex( p1 );
		}
	}
	gl::end();
}

void UVorApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			setFullScreen( !isFullScreen() );
			break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

CINDER_APP_BASIC( UVorApp, RendererGl )

