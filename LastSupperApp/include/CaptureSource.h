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
#pragma once

#include <vector>

#include "cinder/qtime/MovieWriter.h"
#include "cinder/qtime/QuickTime.h"

#include "cinder/Capture.h"
#include "cinder/Surface.h"

#ifndef CINDER_MSW
#define CAPTURE_1394
#endif

#ifdef CAPTURE_1394
#include "Capture1394PParams.h"
#endif

#ifdef CINDER_MSW
#include "CaptureParams.h"
#endif

#include "mndlkit/params/PParams.h"

namespace mndl {

class CaptureSource
{
	public:
		void setup();
		void update();
		void drawParams();

		void shutdown();

		bool isCapturing() const;
		bool checkNewFrame();

		int32_t getWidth() const;
		int32_t getHeight() const;
		ci::Vec2i getSize() const { return ci::Vec2i( getWidth(), getHeight() ); }
		float getAspectRatio() const { return getWidth() / (float)getHeight(); }
		ci::Area getBounds() const { return ci::Area( 0, 0, getWidth(), getHeight() ); }

		ci::Surface8u getSurface();

	protected:
		enum
		{
			SOURCE_RECORDING = 0,
			SOURCE_CAPTURE,
			SOURCE_CAPTURE1394
		};

		void setupParams();
		void playVideoCB();
		void saveVideoCB();

		ci::qtime::MovieSurface mMovie;
		ci::qtime::MovieWriter mMovieWriter;
		bool mSavingVideo;

		int mSource; // recording or camera

		ci::Vec2i mCaptureResolution;

		// qtime capture
#ifdef CINDER_MSW
		std::vector< CaptureParamsRef > mCaptures;
#else
		std::vector< ci::CaptureRef > mCaptures;
#endif
		std::vector< std::string > mDeviceNames;
		int mCurrentCapture;

#ifdef CAPTURE_1394
		// capture1394
		mndl::Capture1394PParamsRef mCapture1394PParams;
#endif
		// params
		mndl::params::PInterfaceGl mParams;
		mndl::params::PInterfaceGl mCaptureSelection;
};

} // namespace mndl;
