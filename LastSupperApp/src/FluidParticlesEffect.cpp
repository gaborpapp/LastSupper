/*
 Copyright (C) 2013 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <vector>

#include <boost/assign/std/vector.hpp>

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/ip/Resize.h"
#include "cinder/Rand.h"

#include "FluidParticlesEffect.h"
#include "GlobalData.h"

using namespace boost::assign;
using namespace ci;
using namespace std;

void FluidParticlesEffect::setup()
{
	GlobalData &gd = GlobalData::get();

	mParams = mndl::params::PInterfaceGl( gd.mControlWindow, "FluidParticles", Vec2i( 310, 300 ), Vec2i( 332, 16 ) );
	mParams.addPersistentSizeAndPosition();

	vector< string > stateNames;
	stateNames += "Interactive", "Rain";
	mState = STATE_INTERACTIVE;
	mParams.addParam( "Fluid state", stateNames, &mState );
	mParams.addSeparator();

	mFluidEnabled = true;
	mParams.addParam( "Fluid particles enabled", &mFluidEnabled );

	vector< std::pair< std::string, boost::any > > vars;
	vars.push_back( make_pair( "Flow multiplier", &mFlowMultiplier ) );
	vars.push_back( make_pair( "Fluid velocity mult", &mFluidVelocityMult ) );
	vars.push_back( make_pair( "Particle max", &mParticleMax ) );
	mParams.addSeparator();
	mParams.addPresets( vars );
	mParams.addSeparator();

	mParams.addText("Optical flow");
	mParams.addPersistentParam( "Flip horizontal", &mFlipHorizontal, true );
	mParams.addPersistentParam( "Flip vertical", &mFlipVertical, true );
	mParams.addPersistentParam( "Draw flow", &mDrawFlow, false );
	mParams.addPersistentParam( "Draw fluid", &mDrawFluid, false );
	mParams.addPersistentParam( "Draw particles", &mDrawParticles, true );
	vector< string > captureStateNames;
	captureStateNames += "Control", "Both", "None";
	mDrawCapture = CAPTURE_CONTROL;
	mParams.addParam( "Draw capture", captureStateNames, &mDrawCapture );
	mParams.addPersistentParam( "Capture alpha", &mCaptureAlpha, .1f, "min=0 max=1 step=0.05" );
	mParams.addPersistentParam( "Flow multiplier", &mFlowMultiplier, .105, "min=.001 max=2 step=.001" );
	mParams.addPersistentParam( "Flow width", &mOptFlowWidth, 160, "min=20 max=640", true );
	mParams.addPersistentParam( "Flow height", &mOptFlowHeight, 120, "min=20 max=480", true );
	mParams.addSeparator();

	mParams.addText( "Particles" );
	mParams.addPersistentParam( "Particle aging", &mParticleAging, 0.97f, "min=0 max=1 step=0.001" );
	mParams.addPersistentParam( "Particle min", &mParticleMin, 0, "min=0 max=50" );
	mParams.addPersistentParam( "Particle max", &mParticleMax, 25, "min=0 max=50" );
	mParams.addPersistentParam( "Velocity max", &mMaxVelocity, 7.f, "min=1 max=100" );
	mParams.addPersistentParam( "Velocity particle multiplier", &mVelParticleMult, .57, "min=0 max=2 step=.01" );
	mParams.addPersistentParam( "Velocity particle min", &mVelParticleMin, 1.f, "min=1 max=100 step=.5" );
	mParams.addPersistentParam( "Velocity particle max", &mVelParticleMax, 60.f, "min=1 max=100 step=.5" );
	mParams.addSeparator();

	mParams.addPersistentParam( "Particle max", &mParticleMax, 25, "min=0 max=50" );
	mParams.addPersistentParam( "Velocity max", &mMaxVelocity, 7.f, "min=1 max=100" );
	mParams.addPersistentParam( "Velocity particle multiplier", &mVelParticleMult, .57, "min=0 max=2 step=.01" );
	mParams.addPersistentParam( "Velocity particle min", &mVelParticleMin, 1.f, "min=1 max=100 step=.5" );
	mParams.addPersistentParam( "Velocity particle max", &mVelParticleMax, 60.f, "min=1 max=100 step=.5" );
	mParams.addSeparator();

	// fluid
	mParams.addText( "Fluid" );
	mParams.addPersistentParam( "Fluid width", &mFluidWidth, 160, "min=16 max=512", true );
	mParams.addPersistentParam( "Fluid height", &mFluidHeight, 120, "min=16 max=512", true );
	mParams.addPersistentParam( "Fade speed", &mFluidFadeSpeed, 0.012f, "min=0 max=1 step=0.0005" );
	mParams.addPersistentParam( "Viscosity", &mFluidViscosity, 0.00003f, "min=0 max=1 step=0.00001" );
	mParams.addPersistentParam( "Delta t", &mFluidDeltaT, 0.4f, "min=0 max=10 step=0.05" );
	mParams.addPersistentParam( "Vorticity confinement", &mFluidVorticityConfinement, false );
	mParams.addPersistentParam( "Wrap x", &mFluidWrapX, true );
	mParams.addPersistentParam( "Wrap y", &mFluidWrapY, true );
	mParams.addPersistentParam( "Fluid color", &mFluidColor, Color( 1.f, 0.05f, 0.01f ) );
	mParams.addPersistentParam( "Fluid velocity mult", &mFluidVelocityMult, 10.f, "min=1 max=50 step=0.5" );
	mParams.addPersistentParam( "Fluid color mult", &mFluidColorMult, .5f, "min=0.05 max=10 step=0.05" );

	mFluidSolver.setup( mFluidWidth, mFluidHeight );
	mFluidSolver.enableRGB( false );
	mFluidSolver.setColorDiffusion( 0 );
	mFluidDrawer.setup( &mFluidSolver );
	mParams.addButton( "Reset fluid", [&]() { mFluidSolver.reset(); } );

	mParams.addSeparator();
	mParams.addText("Post process");
	mParams.addPersistentParam( "Streak attenuation", &mStreakAttenuation, .975f, "min=0 max=.999 step=.001" );
	mParams.addPersistentParam( "Streak iterations", &mStreakIterations, 8, "min=1 max=32" );
	mParams.addPersistentParam( "Streak strength", &mStreakStrength, .8f, "min=0 max=1 step=.05" );

    mParticles.setFluidSolver( &mFluidSolver );

	gl::Fbo::Format format;
	format.enableDepthBuffer( false );
	format.setSamples( 4 );
	mParticlesFbo = gl::Fbo( 1024, 768, format );
	mParticles.setWindowSize( mParticlesFbo.getSize() );

	mKawaseStreak = mndl::gl::fx::KawaseStreak( mParticlesFbo.getWidth(), mParticlesFbo.getHeight() );
	mOptFlowClipRectNorm = Rectf( 0, 0, 1, 1 );
	mIsActive = false;
}

void FluidParticlesEffect::instantiate()
{
	mPrevFrame.release();
	mFluidSolver.reset();
	mIsActive = true;
}

void FluidParticlesEffect::deinstantiate()
{
	mIsActive = false;
}

void FluidParticlesEffect::update()
{
	static int lastState = STATE_INTERACTIVE;

	GlobalData &gd = GlobalData::get();

	cv::Mat currentFrame;
	bool newFrame = false;
	if ( gd.mCaptureSource.isCapturing() && gd.mCaptureSource.checkNewFrame() )
	{
		Surface8u captSurf = Surface8u( Channel8u( gd.mCaptureSource.getSurface() ) );

		Surface8u smallSurface( mOptFlowWidth, mOptFlowHeight, false );
		if ( ( captSurf.getWidth() != mOptFlowWidth ) ||
				( captSurf.getHeight() != mOptFlowHeight ) )
		{
			ip::resize( captSurf, &smallSurface );
		}
		else
		{
			smallSurface = captSurf;
		}

		currentFrame = toOcv( Channel( smallSurface ) );
		if ( mFlipHorizontal || mFlipVertical )
		{
			int flipCode;
			if ( mFlipHorizontal && mFlipVertical )
				flipCode = -1;
			else
			if ( mFlipHorizontal )
				flipCode = 1;
			else
				flipCode = 0;
			cv::flip( currentFrame, currentFrame, flipCode );
		}

		if ( !mCaptureTexture || ( mCaptureTexture.getWidth() != smallSurface.getWidth() ) ||
			( mCaptureTexture.getHeight() != smallSurface.getHeight() ) )
		{
			mCaptureTexture = gl::Texture( Channel8u( fromOcv( currentFrame ) ) );
		}
		else
		{
			mCaptureTexture.update( Channel8u( fromOcv( currentFrame ) ), mCaptureTexture.getBounds() );
		}

		newFrame = true;
	}

	// optical flow
	if ( ( mState == STATE_INTERACTIVE ) && newFrame )
	{
		if ( ( mPrevFrame.data ) &&
			 ( mPrevFrame.size() == cv::Size( mOptFlowWidth, mOptFlowHeight ) ) )
		{
			double pytScale = .5;
			int levels = 5;
			int winSize = 13;
			int iterations = 5;
			int polyN = 5;
			double polySigma = 1.1;
			int flags = cv::OPTFLOW_FARNEBACK_GAUSSIAN;

			cv::calcOpticalFlowFarneback(
					mPrevFrame, currentFrame,
					mFlow,
					pytScale, levels, winSize, iterations, polyN, polySigma, flags );
		}
		mPrevFrame = currentFrame;

		// fluid update
		if ( mFlow.data )
		{
			RectMapping ofNorm( Area( 0, 0, mFlow.cols, mFlow.rows ),
					Rectf( 0.f, 0.f, 1.f, 1.f ) );
			RectMapping normOf( Rectf( 0.f, 0.f, 1.f, 1.f ),
					Area( 0, 0, mFlow.cols, mFlow.rows ) );

			// calculate mask
			Rectf maskRect = normOf.map( mOptFlowClipRectNorm );
			if ( ( maskRect.getWidth() > 0 ) && maskRect.getHeight() > 0 )
			{
				Area maskArea( maskRect );
				for ( int y = maskArea.y1; y < maskArea.y2; y++ )
				{
					for ( int x = maskArea.x1; x < maskArea.x2; x++ )
					{
						Vec2f v = fromOcv( mFlow.at< cv::Point2f >( y, x ) );
						Vec2f p( x + .5, y + .5 );
						addToFluid( ofNorm.map( p ), ofNorm.map( v ) * mFlowMultiplier,
								mFluidEnabled, mFluidEnabled, mFluidEnabled );
					}
				}
			}
		}
	}
	else
	if ( mState == STATE_RAIN )
	{
		if ( lastState == STATE_INTERACTIVE )
		{
			mFluidSolver.reset();
			mFluidVorticityConfinement = false;
			for ( int i = 0; i < 100; i++ )
			{
				Vec2f p( Rand::randFloat(), Rand::randFloat() );
				Vec2f v( 0.f, 0.5f );

				addToFluid( p, v, false, true, false );
			}
		}

		if ( ( Rand::randInt( 128 ) < mParticleMax ) && mFluidEnabled )
		{
			// add one falling particle
			{
				Vec2f p( Rand::randFloat(), 0.f );
				if ( ( p.x >= mOptFlowClipRectNorm.x1 ) &&
				     ( p.x <= mOptFlowClipRectNorm.x2 ) )
				{
					Vec2f v( 0.f, 0.05f );
					v.rotate( Rand::randFloat( -.1f, .1f ) );

					addToFluid( p, v );
				}
			}
		}
	}

	lastState = mState;

	// fluid & particles
	mFluidSolver.setFadeSpeed( mFluidFadeSpeed );
	mFluidSolver.setDeltaT( mFluidDeltaT  );
	mFluidSolver.setVisc( mFluidViscosity );
	mFluidSolver.enableVorticityConfinement( mFluidVorticityConfinement );
	mFluidSolver.setWrap( mFluidWrapX, mFluidWrapY );
	mFluidSolver.update();

	mParticles.setAging( mParticleAging );
	mParticles.update( app::getElapsedSeconds() );
}

void FluidParticlesEffect::drawControl()
{
	GlobalData &gd = GlobalData::get();

	if ( mIsActive )
	{
		RectMapping map( Rectf( 0.f, 0.f, 1.f, 1.f ), gd.mPreviewRect );

		Rectf maskRect = map.map( mOptFlowClipRectNorm );
		gl::enableAlphaBlending();
		gl::color( ColorA( 1.f, 0.f, 0.f, .2f ) );
		gl::drawSolidRect( maskRect );
		gl::disableAlphaBlending();
		gl::color( Color::white() );

		// capture only in control window
		if ( ( mDrawCapture == CAPTURE_CONTROL ) &&
				 mCaptureTexture )
		{
			gl::enableAdditiveBlending();
			// FIXME: processed texture should be here if the filter fbo was not flipped
			Vec2f offset = gd.mMaskRect->getOffset() * gd.mPreviewRect.getSize();
			gl::pushModelView();
			gl::translate( offset );
			mCaptureTexture.enableAndBind();
			gl::color( ColorA( 1, 1, 1, mCaptureAlpha ) );
			gl::drawSolidRect( gd.mPreviewRect );
			mCaptureTexture.unbind();
			gl::popModelView();
			gl::color( Color::white() );
			gl::disableAlphaBlending();
		}
	}

	mParams.draw();
}

void FluidParticlesEffect::addToFluid( const Vec2f &pos, const Vec2f &vel, bool addParticles, bool addForce, bool addColor )
{
	Vec2f p;
	if ( vel.lengthSquared() > 0.000001f )
	{
		p.x = constrain( pos.x, 0.0f, 1.0f );
		p.y = constrain( pos.y, 0.0f, 1.0f );

		if ( addParticles )
		{
			int count = static_cast<int>(
					lmap<float>( vel.length() * mVelParticleMult * mParticlesFbo.getWidth(),
						mVelParticleMin, mVelParticleMax,
						mParticleMin, mParticleMax ) );
			if ( count > 0 )
			{
				if ( addParticles )
					mParticles.addParticle( p * Vec2f( mParticlesFbo.getSize() ), count );
			}
		}
		if ( addForce )
			mFluidSolver.addForceAtPos( p, vel * mFluidVelocityMult );

		if ( addColor )
		{
			mFluidSolver.addColorAtPos( p, Color::white() * mFluidColorMult );
		}
	}
}

void FluidParticlesEffect::draw()
{
	gl::clear();

	gl::setViewport( getBounds() );
	gl::setMatricesWindow( getSize() );

	if ( mDrawFluid )
	{
		gl::color( mFluidColor );
		mFluidDrawer.draw( 0, 0, getWidth(), getHeight() );
	}

	if ( mDrawParticles )
	{
		{
			gl::SaveFramebufferBinding bindingSaver;

			mParticlesFbo.bindFramebuffer();
			// FIXME: particles overwrite fluid
			//gl::clear( ColorA::gray( 0, 0 ) );
			gl::clear();

			gl::setViewport( mParticlesFbo.getBounds() );
			gl::setMatricesWindow( mParticlesFbo.getSize(), false );

			mParticles.draw();
			mParticlesFbo.unbindFramebuffer();
		}

		gl::Texture output = mKawaseStreak.process( mParticlesFbo.getTexture(), mStreakAttenuation, mStreakIterations, mStreakStrength );
		{
			gl::setViewport( getBounds() );
			gl::setMatricesWindow( getSize() );
			if ( mDrawFluid )
				gl::enableAlphaBlending();
			gl::color( Color::white() );
			gl::draw( output, getBounds() );
		}
		if ( mDrawFluid )
			gl::disableAlphaBlending();
	}

	// draw output to window
	if ( ( mDrawCapture == CAPTURE_BOTH ) && mCaptureTexture )
	{
		gl::enableAdditiveBlending();
		gl::color( ColorA( 1, 1, 1, mCaptureAlpha ) );
		mCaptureTexture.enableAndBind();

		gl::drawSolidRect( getBounds() );
		mCaptureTexture.unbind();
		gl::color( Color::white() );
		gl::disableAlphaBlending();
	}

	// flow vectors, TODO: make this faster using Vbo
	gl::disable( GL_TEXTURE_2D );
	if ( mDrawFlow && mFlow.data )
	{
		RectMapping ofToWin( Area( 0, 0, mFlow.cols, mFlow.rows ),
				getBounds() );
		float ofScale = mFlowMultiplier * getWidth() / (float)mOptFlowWidth;
		gl::color( Color::white() );
		for ( int y = 0; y < mFlow.rows; y++ )
		{
			for ( int x = 0; x < mFlow.cols; x++ )
			{
				Vec2f v = fromOcv( mFlow.at< cv::Point2f >( y, x ) );
				Vec2f p( x + .5, y + .5 );
				gl::drawLine( ofToWin.map( p ),
						ofToWin.map( p + ofScale * v ) );
			}
		}
	}
}

void FluidParticlesEffect::mouseDown( app::MouseEvent event )
{
	GlobalData &gd = GlobalData::get();
	if ( app::getWindow() == gd.mControlWindow )
		mMouseStartNorm = Vec2f( event.getPos() ) / Vec2f( getSize() );
}

void FluidParticlesEffect::mouseDrag( app::MouseEvent event )
{
	GlobalData &gd = GlobalData::get();
	if ( app::getWindow() == gd.mControlWindow )
	{
		mMouseEndNorm = Vec2f( event.getPos() ) / Vec2f( getSize() );
		float x0 = math< float >::min( mMouseStartNorm.x, mMouseEndNorm.x );
		float x1 = math< float >::max( mMouseStartNorm.x, mMouseEndNorm.x );
		mOptFlowClipRectNorm = Rectf( x0, 0.f, x1, 1.f );
	}
}

void FluidParticlesEffect::mouseUp( app::MouseEvent event )
{
	mouseDrag( event );
}

