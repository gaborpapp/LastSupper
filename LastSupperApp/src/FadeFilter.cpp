#include "cinder/app/App.h"
#include "cinder/gl/gl.h"

#include "FadeFilter.h"
#include "GlobalData.h"

using namespace ci;
using namespace ci::app;

#define STRINGIFY(x) #x

const char *FadeFilter::sFadeVertexShader =
	STRINGIFY(
		void main()
		{
			gl_FrontColor = gl_Color;
			gl_TexCoord[ 0 ] = gl_MultiTexCoord0;
			gl_Position = ftransform();
		}
);

const char *FadeFilter::sFadeFragmentShader =
	STRINGIFY(
		uniform sampler2D tex;
		uniform vec3 fadeColor;
		uniform float fadeValue;
		void main()
		{
			vec3 color = texture2D( tex, gl_TexCoord[ 0 ].st ).rgb;
			gl_FragColor = vec4( mix( color, fadeColor, fadeValue ), 1. );
		}
);

FadeFilter::FadeFilter( int w, int h )
{
	GlobalData &gd = GlobalData::get();

	mFade = 0;
	gd.mPostProcessingParams.addPersistentParam( "FadeFilter enable", &mEnabled, false, "group=FadeFilter" );
	gd.mPostProcessingParams.addPersistentParam( "Fade duration", &mFadeDuration, 2.f, "min=.5 step=.25 group=FadeFilter" );
	gd.mPostProcessingParams.addPersistentParam( "Fade color", &mFadeColor, Color::white(), "group=FadeFilter" );
	gd.mPostProcessingParams.addButton( "Fade color white", [&]() { mFadeColor = Color::white(); }, "group=FadeFilter" );
	gd.mPostProcessingParams.addButton( "Fade color black", [&]() { mFadeColor = Color::black(); }, "group=FadeFilter" );
	gd.mPostProcessingParams.addButton( "Fade out", [&]() {
			mFade = 0.f;
			app::timeline().apply( &mFade, 1.f, mFadeDuration );
			}, "group=FadeFilter" );
	gd.mPostProcessingParams.addButton( "Fade in", [&]() {
			mFade = 1.f;
			app::timeline().apply( &mFade, 0.f, mFadeDuration );
			}, "group=FadeFilter" );
	gd.mPostProcessingParams.setOptions( "FadeFilter", "opened=false" );

	gl::Fbo::Format fboFormat;
	fboFormat.enableDepthBuffer( false );
	fboFormat.setSamples( 4 );
	mFbo = gl::Fbo( w, h, fboFormat );

	try
	{
		mShader = ci::gl::GlslProg( sFadeVertexShader, sFadeFragmentShader );
		mShader.bind();
		mShader.uniform( "tex", 0 );
		mShader.unbind();
	}
	catch ( ci::gl::GlslProgCompileExc &exc )
	{
		app::console() << exc.what() << std::endl;
	}
}


ci::gl::Texture FadeFilter::process( const ci::gl::Texture &source )
{
	ci::gl::SaveFramebufferBinding bindingSaver;

	if ( !mEnabled )
		return source;

	Area viewport = ci::gl::getViewport();
	ci::gl::pushMatrices();

	ci::gl::disableDepthRead();
	ci::gl::disableDepthWrite();

	ci::gl::color( Color::white() );

	mFbo.bindFramebuffer();
	ci::gl::setViewport( mFbo.getBounds() );
	ci::gl::setMatricesWindow( mFbo.getSize(), false );

	if ( mShader )
	{
		mShader.bind();
		mShader.uniform( "fadeValue", mFade );
		mShader.uniform( "fadeColor", mFadeColor );
	}

	source.bind();
	gl::drawSolidRect( mFbo.getBounds() );
	source.unbind();

	if ( mShader )
		mShader.unbind();

	ci::gl::popMatrices();
	ci::gl::setViewport( viewport );

	return mFbo.getTexture();
}

