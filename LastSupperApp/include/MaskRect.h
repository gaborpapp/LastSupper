#pragma once

#include "cinder/Cinder.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Color.h"

class MaskRect;
typedef std::shared_ptr< MaskRect > MaskRectRef;

class MaskRect
{
	public:
		static MaskRectRef create( int w, int h ) { return MaskRectRef( new MaskRect( w, h ) ); }

		ci::gl::Texture process( const ci::gl::Texture &source );

		bool isEnabled() const { return mEnabled; }

		ci::Vec2f getOffset() const  { return mOffset; }

	private:
		MaskRect( int w, int h );

		bool mEnabled;

		float mRectX1, mRectX2;
		float mRectY1, mRectY2;

		ci::Vec2f mOffset;

		ci::Color mColor;
		ci::gl::Fbo mFbo;
};
