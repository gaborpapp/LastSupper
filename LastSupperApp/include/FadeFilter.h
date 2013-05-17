#pragma once

#include "cinder/Cinder.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/Color.h"
#include "cinder/Timeline.h"

class FadeFilter;
typedef std::shared_ptr< FadeFilter > FadeFilterRef;

class FadeFilter
{
	public:
		static FadeFilterRef create( int w, int h ) { return FadeFilterRef( new FadeFilter( w, h ) ); }

		ci::gl::Texture process( const ci::gl::Texture &source );

		bool isEnabled() const { return mEnabled; }

	private:
		FadeFilter( int w, int h );

		bool mEnabled;

		ci::gl::Fbo mFbo;

		ci::Anim< float > mFade;
		float mFadeDuration;
		ci::Color mFadeColor;

		ci::gl::GlslProg mShader;

		static const char *sFadeVertexShader;
		static const char *sFadeFragmentShader;
};
