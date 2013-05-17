#pragma once

#include "cinder/app/App.h"
#include "cinder/Rect.h"

#include "mndlkit/params/PParams.h"

#include "CaptureSource.h"
#include "MaskRect.h"

class GlobalData
{
	private:
		//! Singleton implementation
		GlobalData() {}

	public:
		static GlobalData & get() { static GlobalData data; return data; }

		ci::app::WindowRef mOutputWindow;
		ci::app::WindowRef mControlWindow;
		ci::Rectf mPreviewRect;

		mndl::CaptureSource mCaptureSource;
		mndl::params::PInterfaceGl mPostProcessingParams;

		MaskRectRef mMaskRect;
};
