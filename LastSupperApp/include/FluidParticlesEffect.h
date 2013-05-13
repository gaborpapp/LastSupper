#pragma once

#include <string>
#include <vector>

#include "cinder/gl/Texture.h"

#include "ciMsaFluidDrawerGl.h"
#include "ciMsaFluidSolver.h"
#include "CinderOpenCV.h"

#include "Effect.h"
#include "FluidParticles.h"
#include "KawaseStreak.h"

class FluidParticlesEffect;
typedef std::shared_ptr< FluidParticlesEffect > FluidParticlesEffectRef;

class FluidParticlesEffect : public Effect
{
	public:
		static FluidParticlesEffectRef create() { return FluidParticlesEffectRef( new FluidParticlesEffect() ); }

		void mouseDown( ci::app::MouseEvent event );
		void mouseDrag( ci::app::MouseEvent event );
		void mouseUp( ci::app::MouseEvent event );
		void mouseMove( ci::app::MouseEvent event );

		void setup();

		void instantiate();
		void deinstantiate();

		void update();
		void draw();
		void drawControl();

	private:
		FluidParticlesEffect() : Effect( "Fluid particles" ) {}

		ci::gl::Texture mCaptureTexture;

		// optflow
		bool mFlip;
		bool mDrawFlow;
		bool mDrawFluid;
		bool mDrawParticles;
		bool mDrawCapture;
		float mCaptureAlpha;
		float mFlowMultiplier;

		cv::Mat mPrevFrame;
		cv::Mat mFlow;

		int mOptFlowWidth;
		int mOptFlowHeight;

		// fluid
		bool mFluidEnabled;

		ciMsaFluidSolver mFluidSolver;
		ciMsaFluidDrawerGl mFluidDrawer;

		int mFluidWidth, mFluidHeight;
		float mFluidFadeSpeed;
		float mFluidDeltaT;
		float mFluidViscosity;
		bool mFluidVorticityConfinement;
		bool mFluidWrapX, mFluidWrapY;
		float mFluidVelocityMult;
		float mFluidColorMult;
		ci::Color mFluidColor;

		// particles
		ci::gl::Fbo mParticlesFbo;
		FluidParticleManager mParticles;
		float mParticleAging;
		int mParticleMin;
		int mParticleMax;
		float mMaxVelocity;
		float mVelParticleMult;
		float mVelParticleMin;
		float mVelParticleMax;

		void addToFluid( const ci::Vec2f &pos, const ci::Vec2f &vel, bool addParticles = true, bool addForce = true, bool addColor = true );

		mndl::gl::fx::KawaseStreak mKawaseStreak;

		float mStreakAttenuation;
		int mStreakIterations;
		float mStreakStrength;

		ci::Rectf mOptFlowClipRectNorm;
		ci::Vec2f mMouseStartNorm, mMouseEndNorm;
		bool mIsActive;
};

