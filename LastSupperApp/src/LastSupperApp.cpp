/*
 Copyright (C) 2013 Gabor Papp, Gabor Botond Barna

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

#include <vector>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"

#include "mndlkit/params/PParams.h"

#include "Effect.h"
#include "GlobalData.h"

#include "BlackEffect.h"
#include "FadeFilter.h"
#include "FluidParticlesEffect.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class LastSupperApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void shutdown();

		void keyDown( app::KeyEvent event );
		void mouseDown( app::MouseEvent event );
		void mouseDrag( app::MouseEvent event );
		void mouseMove( app::MouseEvent event );
		void mouseUp( app::MouseEvent event );

		void update();
		void draw();

		void resize();

	private:
		mndl::params::PInterfaceGl mParams;

		float mFps;
		bool mVerticalSyncEnabled;

		bool mapMouseEvent( MouseEvent event, MouseEvent *newEvent );

		void drawControl();
		void drawOutput();

		vector< EffectRef > mEffects;
		int mEffectIndex;
		int mPrevEffectIndex;

		FadeFilterRef mFadeFilter;

#define FBO_WIDTH 1024
#define FBO_HEIGHT 768
		gl::Fbo mFbo;
		int mFboOutputAttachment;

		Vec2i mOutputWindowInitialPos;
		bool mOutputWindowInitialFullscreen;
};

void LastSupperApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1024, 768 );
}

void LastSupperApp::setup()
{
	mndl::params::PInterfaceGl::load( string( "params.xml" ) );

	GlobalData &gd = GlobalData::get();
	gd.mOutputWindow = getWindow();
	gd.mControlWindow = createWindow( Window::Format().size( 1250, 700 ) );

	gd.mPostProcessingParams = mndl::params::PInterfaceGl( gd.mControlWindow, "Postprocessing", Vec2i( 200, 310 ), Vec2i( 516, 342 ) );
	gd.mPostProcessingParams.addPersistentSizeAndPosition();

	mParams = mndl::params::PInterfaceGl( gd.mControlWindow, "Parameters", Vec2i( 200, 310 ), Vec2i( 16, 16 ) );
	mParams.addPersistentSizeAndPosition();
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addPersistentParam( "Vertical sync", &mVerticalSyncEnabled, false );
	mParams.addSeparator();
	mParams.addPersistentParam( "Output fullscreen", &mOutputWindowInitialFullscreen, false );
	mParams.addPersistentParam( "Output pos x", &mOutputWindowInitialPos.x, 0 );
	mParams.addPersistentParam( "Output pos y", &mOutputWindowInitialPos.y, 0 );
	mParams.addSeparator();

	gd.mOutputWindow->setPos( mOutputWindowInitialPos );
	gd.mOutputWindow->setFullScreen( mOutputWindowInitialFullscreen );

	gd.mCaptureSource.setup();

	// output fbo
	gl::Fbo::Format format;
	format.setSamples( 4 );
	format.enableColorBuffer( true, 2 );
	mFbo = gl::Fbo( FBO_WIDTH, FBO_HEIGHT, format );
	mFboOutputAttachment = 0;

	// postprocessing filters
	gd.mMaskRect = MaskRect::create( mFbo.getWidth(), mFbo.getHeight() );
	mFadeFilter = FadeFilter::create( mFbo.getWidth(), mFbo.getHeight() );

	// setup effects
	mEffects.push_back( BlackEffect::create() );
	mEffects.push_back( FluidParticlesEffect::create() );

	vector< string > effectNames;
	for ( auto it = mEffects.cbegin(); it != mEffects.cend(); ++it )
	{
		effectNames.push_back( (*it)->getName() );
		(*it)->resize( mFbo.getSize() );
		(*it)->setup();
	}
	mEffectIndex = mPrevEffectIndex = 0;
	mParams.addParam( "Effect", effectNames, &mEffectIndex );
	mParams.addSeparator();

	mndl::params::PInterfaceGl::showAllParams( true, true );
}

void LastSupperApp::update()
{
	mFps = getAverageFps();
	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );

	GlobalData &gd = GlobalData::get();

	if ( gd.mOutputWindow->getPos() != mOutputWindowInitialPos )
	{
		mOutputWindowInitialPos = gd.mOutputWindow->getPos();
	}

	gd.mCaptureSource.update();

	// update current effect
	if ( mEffectIndex != mPrevEffectIndex )
	{
		mEffects[ mPrevEffectIndex ]->deinstantiate();
		mEffects[ mEffectIndex ]->instantiate();
	}
	mEffects[ mEffectIndex ]->update();
	mPrevEffectIndex = mEffectIndex;
}

void LastSupperApp::draw()
{
	GlobalData &gd = GlobalData::get();
	app::WindowRef currentWindow = getWindow();

	if ( currentWindow == gd.mOutputWindow )
	{
		drawOutput();
	}
	else
	if ( currentWindow == gd.mControlWindow )
	{
		drawControl();
	}
}

void LastSupperApp::drawOutput()
{
	GlobalData &gd = GlobalData::get();

	mFbo.bindFramebuffer();
	gl::clear();
	mEffects[ mEffectIndex ]->draw();
	mFbo.unbindFramebuffer();

	mFboOutputAttachment = 0;
	if ( gd.mMaskRect->isEnabled() )
	{
		gl::Texture processed = gd.mMaskRect->process( mFbo.getTexture( mFboOutputAttachment ) );
		mFbo.bindFramebuffer();
		glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT + ( mFboOutputAttachment ^ 1 ) );
		gl::setViewport( mFbo.getBounds() );
		gl::setMatricesWindow( mFbo.getSize(), false );
		gl::color( Color::white() );
		gl::draw( processed, mFbo.getBounds() );
		mFbo.unbindFramebuffer();
		mFboOutputAttachment ^= 1;
	}
	if ( mFadeFilter->isEnabled() )
	{
		gl::Texture processed = mFadeFilter->process( mFbo.getTexture( mFboOutputAttachment ) );
		mFbo.bindFramebuffer();
		glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT + ( mFboOutputAttachment ^ 1 ) );
		gl::setViewport( mFbo.getBounds() );
		gl::setMatricesWindow( mFbo.getSize(), false );
		gl::color( Color::white() );
		gl::draw( processed, mFbo.getBounds() );
		mFbo.unbindFramebuffer();
		mFboOutputAttachment ^= 1;
	}

	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );
	gl::clear();
	gl::color( Color::white() );

	// flip fbo texture
	gl::pushModelView();
	gl::translate( 0.f, getWindowHeight() );
	gl::scale( 1.f, -1.f );
	gl::draw( mFbo.getTexture( mFboOutputAttachment ), getWindowBounds() );
	gl::popModelView();
}

void LastSupperApp::drawControl()
{
	GlobalData &gd = GlobalData::get();

	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );
	gl::clear();

	// draw preview
	gl::color( Color::white() );
	// flip fbo texture
	gl::pushModelView();
	gl::translate( 0.f, gd.mPreviewRect.getHeight() + 2 * 16 );
	gl::scale( 1.f, -1.f );
	gl::draw( mFbo.getTexture( mFboOutputAttachment ), gd.mPreviewRect );
	gl::popModelView();
	gl::drawString( "Preview", gd.mPreviewRect.getUpperLeft() + Vec2f( 16, 16 ) );
	gl::drawStrokedRect( gd.mPreviewRect );

	mParams.draw();
	for ( auto it = mEffects.cbegin(); it != mEffects.cend(); ++it )
	{
		(*it)->drawControl();
	}

	GlobalData::get().mCaptureSource.drawParams();
	GlobalData::get().mPostProcessingParams.draw();
}

void LastSupperApp::resize()
{
	if ( getWindow() == GlobalData::get().mControlWindow )
	{
		// change preview rect
		const int b = 16;
		const int pw = int( getWindowWidth() * .4f );
		const int ph = int( pw / ( FBO_WIDTH / (float)FBO_HEIGHT ) );

		GlobalData::get().mPreviewRect = Rectf( getWindowWidth() - pw - b, b, getWindowWidth() - b, ph + b );
	}
}

bool LastSupperApp::mapMouseEvent( MouseEvent event, MouseEvent *newEvent )
{
	GlobalData &gd = GlobalData::get();
	RectMapping mapping;

	if ( event.getWindow() == GlobalData::get().mControlWindow )
	{
		if ( !gd.mPreviewRect.contains( Vec2f( event.getPos() ) ) )
			return false;
		// map event from preview to fbo area
		mapping = RectMapping( gd.mPreviewRect, mFbo.getBounds() );
	}
	else
	{
		// map event from output window to fbo area
		mapping = RectMapping( GlobalData::get().mOutputWindow->getBounds(), mFbo.getBounds() );
	}

	Vec2f mappedPos = mapping.map( Vec2f( event.getPos() ) );
	*newEvent = app::MouseEvent( event.getWindow(),
			( (unsigned)event.isLeft() * MouseEvent::LEFT_DOWN ) |
			( (unsigned)event.isMiddle() * MouseEvent::MIDDLE_DOWN ) |
			( (unsigned)event.isRight() * MouseEvent::RIGHT_DOWN ),
			(int)mappedPos.x, (int)mappedPos.y,
			( (unsigned)event.isLeftDown() * MouseEvent::LEFT_DOWN ) |
			( (unsigned)event.isMiddleDown() * MouseEvent::MIDDLE_DOWN ) |
			( (unsigned)event.isRightDown() * MouseEvent::RIGHT_DOWN ) |
			( (unsigned)event.isShiftDown() * MouseEvent::SHIFT_DOWN ) |
			( (unsigned)event.isAltDown() * MouseEvent::ALT_DOWN ) |
			( (unsigned)event.isControlDown() * MouseEvent::CTRL_DOWN ) |
			( (unsigned)event.isMetaDown() * MouseEvent::META_DOWN ) |
			( (unsigned)event.isAccelDown() * MouseEvent::ACCEL_DOWN ),
			event.getWheelIncrement(),
			event.getNativeModifiers() );

	return true;
}

void LastSupperApp::mouseDown( MouseEvent event )
{
	MouseEvent newEvent;
	if ( mapMouseEvent( event, &newEvent ) )
		mEffects[ mEffectIndex ]->mouseDown( newEvent );
}

void LastSupperApp::mouseDrag( ci::app::MouseEvent event )
{
	MouseEvent newEvent;
	if ( mapMouseEvent( event, &newEvent ) )
		mEffects[ mEffectIndex ]->mouseDrag( newEvent );
}

void LastSupperApp::mouseMove( ci::app::MouseEvent event )
{
	MouseEvent newEvent;
	if ( mapMouseEvent( event, &newEvent ) )
		mEffects[ mEffectIndex ]->mouseMove( newEvent );
}

void LastSupperApp::mouseUp( ci::app::MouseEvent event )
{
	MouseEvent newEvent;
	if ( mapMouseEvent( event, &newEvent ) )
		mEffects[ mEffectIndex ]->mouseUp( newEvent );
}

void LastSupperApp::keyDown( KeyEvent event )
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

void LastSupperApp::shutdown()
{
	mndl::params::PInterfaceGl::save();
	for ( auto it = mEffects.cbegin(); it != mEffects.cend(); ++it )
	{
		(*it)->shutdown();
	}
	GlobalData::get().mCaptureSource.shutdown();
}

CINDER_APP_BASIC( LastSupperApp, RendererGl )

