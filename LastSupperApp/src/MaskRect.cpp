#include "cinder/app/App.h"
#include "cinder/gl/gl.h"

#include "GlobalData.h"
#include "MaskRect.h"

using namespace ci;

MaskRect::MaskRect( int w, int h )
{
	GlobalData &gd = GlobalData::get();

	gd.mPostProcessingParams.addPersistentParam( "MaskRect enable", &mEnabled, false, "group=MaskRect" );
	gd.mPostProcessingParams.addPersistentParam( "MaskRect color", &mColor, Color::black(), "group=MaskRect" );
	gd.mPostProcessingParams.addPersistentParam( "MaskRect left", &mRectX1, 0.f, "min=0 max=1 step=.001 group=MaskRect" );
	gd.mPostProcessingParams.addPersistentParam( "MaskRect right", &mRectX2, 1.f, "min=0 max=1 step=.001 group=MaskRect" );
	gd.mPostProcessingParams.addPersistentParam( "MaskRect top", &mRectY1, 1.f, "min=0 max=1 step=.001 group=MaskRect" );
	gd.mPostProcessingParams.addPersistentParam( "MaskRect bottom", &mRectY2, 0.f, "min=0 max=1 step=.001 group=MaskRect" );
	gd.mPostProcessingParams.setOptions( "MaskRect", "opened=false" );

	gl::Fbo::Format fboFormat;
	fboFormat.enableDepthBuffer( false );
	fboFormat.setSamples( 4 );
	mFbo = gl::Fbo( w, h, fboFormat );
}

gl::Texture MaskRect::process( const gl::Texture &source )
{
	gl::SaveFramebufferBinding bindingSaver;

	if ( !mEnabled )
		return source;

	gl::pushMatrices();
	mFbo.bindFramebuffer();
	gl::setViewport( mFbo.getBounds() );
	gl::setMatricesWindow( mFbo.getSize(), false );

	RectMapping normalizedToDestination( Rectf( 0, 0, 1, 1 ), mFbo.getBounds() );
	gl::draw( source, mFbo.getBounds() );
	gl::color( mColor );
	gl::drawSolidRect( normalizedToDestination.map( Rectf( 0, 0, mRectX1, 1 ) ) );
	gl::drawSolidRect( normalizedToDestination.map( Rectf( mRectX2, 0, 1, 1 ) ) );
	gl::drawSolidRect( normalizedToDestination.map( Rectf( 0, 0, 1, mRectY2 ) ) );
	gl::drawSolidRect( normalizedToDestination.map( Rectf( 0, mRectY1, 1, 1 ) ) );

	/*
	// FIXME: do this with rects
	gl::clear( mColor );
	Rectf normRect( mRectX1, mRectY1, mRectX2, mRectY2 );
	RectMapping normalizedToSource( Rectf( 0, 0, 1, 1 ), source.getBounds() );
	RectMapping normalizedToDestination( Rectf( 0, 0, 1, 1 ), mFbo.getBounds() );
	Area srcArea( normalizedToSource.map( normRect ) );
	Rectf dstRect = normalizedToDestination.map( normRect );
	gl::draw( source, srcArea, dstRect );
	*/

	mFbo.unbindFramebuffer();
	gl::popMatrices();

	return mFbo.getTexture();
}

