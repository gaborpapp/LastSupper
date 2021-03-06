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

#include "boost/date_time.hpp"

#include "cinder/app/App.h"

#include "CaptureSource.h"
#include "GlobalData.h"

using namespace std;

namespace mndl {

void CaptureSource::setup()
{
#ifdef CAPTURE_1394
	// capture1394
	try
	{
		mCapture1394PParams = Capture1394PParams::create();
	}
	catch( Capture1394Exc &exc )
	{
		ci::app::console() << exc.what() << endl;
		ci::app::App::get()->quit();
	}
#endif

	GlobalData &gd = GlobalData::get();
	mParams = mndl::params::PInterfaceGl( gd.mControlWindow, "Capture Source", ci::Vec2i( 310, 90 ), ci::Vec2i( 16, 326 ) );
	mParams.addPersistentSizeAndPosition();
	mCaptureSelection = mndl::params::PInterfaceGl( gd.mControlWindow, "Capture", ci::Vec2i( 310, 90 ), ci::Vec2i( 16, 432 ) );
	mCaptureSelection.addPersistentParam( "Capture width", &mCaptureResolution.x, 160, "", true );
	mCaptureSelection.addPersistentParam( "Capture height", &mCaptureResolution.y, 120, "", true );

	// capture
	// list out the capture devices
	vector< ci::Capture::DeviceRef > devices( ci::Capture::getDevices() );

	for ( auto deviceIt = devices.cbegin(); deviceIt != devices.cend(); ++deviceIt )
	{
		ci::Capture::DeviceRef device = *deviceIt;
		string deviceName = device->getName(); // + " " + device->getUniqueId();

		try
		{
			if ( device->checkAvailable() )
			{
#ifdef CINDER_MSW
				mCaptures.push_back( CaptureParams::create( mCaptureResolution.x, mCaptureResolution.y, device ) );
#else
				mCaptures.push_back( ci::Capture::create( mCaptureResolution.x, mCaptureResolution.y, device ) );
#endif
				mDeviceNames.push_back( deviceName );
			}
			else
			{
#ifdef CINDER_MSW
				mCaptures.push_back( CaptureParamsRef() );
#else
				mCaptures.push_back( ci::CaptureRef() );
#endif
				mDeviceNames.push_back( deviceName + " not available" );
			}
		}
		catch ( ci::CaptureExc & )
		{
			ci::app::console() << "Unable to initialize device: " << device->getName() << endl;
		}
	}

	if ( mDeviceNames.empty() )
	{
		mDeviceNames.push_back( "Camera not available" );
#ifdef CINDER_MSW
		mCaptures.push_back( CaptureParamsRef() );
#else
		mCaptures.push_back( ci::CaptureRef() );
#endif
	}

	mCaptureSelection.addPersistentSizeAndPosition();
	mCaptureSelection.addPersistentParam( "Camera", mDeviceNames, &mCurrentCapture, 0 );

	if ( mCurrentCapture >= (int)mCaptures.size() )
		mCurrentCapture = 0;
	setupParams();
#ifdef CINDER_MSW
	CaptureParams::setup();
#endif
}

void CaptureSource::setupParams()
{
	mndl::params::PInterfaceGl::save();
	mParams.clear();

	vector< string > enumNames;
	enumNames.push_back( "Recording" );
	enumNames.push_back( "Capture" );
#ifdef CAPTURE_1394
	enumNames.push_back( "Capture1394" );
#endif
	mParams.addPersistentParam( "Source", enumNames, &mSource, SOURCE_CAPTURE );

	mParams.addSeparator();
	if ( mSource == SOURCE_RECORDING )
	{
		mParams.addButton( "Play video", std::bind( &CaptureSource::playVideoCB, this ) );
	}
	else
	{
		mParams.addButton( "Save video", std::bind( &CaptureSource::saveVideoCB, this ) );
	}
}

void CaptureSource::update()
{
	static int lastCapture = -1;
	static int lastSource = -1;
	bool resetParams = false;

	// change gui buttons if switched between capture and playback
	if ( lastSource != mSource )
	{
		setupParams();
#ifdef CINDER_MSW
		CaptureParams::removeParams();
#endif
		resetParams = true;
		lastSource = mSource;
	}

	// capture
	if ( mSource == SOURCE_CAPTURE )
	{
		// stop and start capture devices
		if ( lastCapture != mCurrentCapture )
		{
			resetParams = true;

			if ( ( lastCapture >= 0 ) && ( mCaptures[ lastCapture ] ) )
				mCaptures[ lastCapture ]->stop();

			if ( mCaptures[ mCurrentCapture ] )
				mCaptures[ mCurrentCapture ]->start();

			lastCapture = mCurrentCapture;
		}

#ifdef CINDER_MSW
		if ( resetParams )
			mCaptures[ mCurrentCapture ]->buildParams();
		else
			mCaptures[ mCurrentCapture ]->updateParams();
#endif
	}
	else // SOURCE_RECORDING or SOURCE_CAPTURE1394
	{
		// stop capture device
		if ( ( lastCapture != -1 ) && ( mCaptures[ lastCapture ] ) )
		{
			mCaptures[ lastCapture ]->stop();
			lastCapture = -1;
		}

#ifdef CAPTURE_1394
		if ( mSource == SOURCE_CAPTURE1394 )
		{
			try
			{
				mCapture1394PParams->update();
			}
			catch( Capture1394Exc &exc )
			{
				ci::app::console() << exc.what() << endl;
			}
		}
#endif
	}
}

void CaptureSource::shutdown()
{
	if ( ( mSource == SOURCE_CAPTURE ) && ( mCaptures[ mCurrentCapture ] ) )
		mCaptures[ mCurrentCapture ]->stop();
}

bool CaptureSource::isCapturing() const
{
	switch ( mSource )
	{
		case SOURCE_CAPTURE:
			return mCaptures[ mCurrentCapture ].get() != 0;
			break;

#ifdef CAPTURE_1394
		case SOURCE_CAPTURE1394:
			return mCapture1394PParams->getCurrentCaptureRef().get() != 0;
			break;
#endif

		case SOURCE_RECORDING:
			return mMovie;
			break;

		default:
			return false;
			break;
	}
}

bool CaptureSource::checkNewFrame()
{
	switch ( mSource )
	{
		case SOURCE_CAPTURE:
			return mCaptures[ mCurrentCapture ]->checkNewFrame();
			break;

#ifdef CAPTURE_1394
		case SOURCE_CAPTURE1394:
			return mCapture1394PParams->getCurrentCaptureRef()->checkNewFrame();
			break;
#endif

		case SOURCE_RECORDING:
			return mMovie.checkNewFrame();
			break;

		default:
			return false;
			break;
	}
}

int32_t CaptureSource::getWidth() const
{
	switch ( mSource )
	{
		case SOURCE_CAPTURE:
			return mCaptures[ mCurrentCapture ]->getWidth();
			break;

#ifdef CAPTURE_1394
		case SOURCE_CAPTURE1394:
			return mCapture1394PParams->getCurrentCaptureRef()->getWidth();
			break;
#endif

		case SOURCE_RECORDING:
			return mMovie.getWidth();
			break;

		default:
			return 0;
			break;
	}
}

int32_t CaptureSource::getHeight() const
{
	switch ( mSource )
	{
		case SOURCE_CAPTURE:
			return mCaptures[ mCurrentCapture ]->getHeight();
			break;

#ifdef CAPTURE_1394
		case SOURCE_CAPTURE1394:
			return mCapture1394PParams->getCurrentCaptureRef()->getHeight();
			break;
#endif

		case SOURCE_RECORDING:
			return mMovie.getHeight();
			break;

		default:
			return 0;
			break;
	}
}

ci::Surface8u CaptureSource::getSurface()
{
	ci::Surface8u surface;
	switch ( mSource )
	{
		case SOURCE_CAPTURE:
			surface = mCaptures[ mCurrentCapture ]->getSurface();
			break;

#ifdef CAPTURE_1394
		case SOURCE_CAPTURE1394:
			surface = mCapture1394PParams->getCurrentCaptureRef()->getSurface();
			break;
#endif

		case SOURCE_RECORDING:
			surface = mMovie.getSurface();
			break;

		default:
			break;
	}

	if ( mSavingVideo )
	{
		mMovieWriter.addFrame( surface );
	}

	return surface;
}

void CaptureSource::saveVideoCB()
{
	if ( mSavingVideo )
	{
		mParams.setOptions( "Save video", "label=`Save video`" );
		mMovieWriter.finish();
	}
	else
	{
		mParams.setOptions( "Save video", "label=`Finish saving`" );

		ci::qtime::MovieWriter::Format format;
		format.setCodec( ci::qtime::MovieWriter::CODEC_H264 );
		format.setQuality( 0.5f );
		format.setDefaultDuration( 1. / 25. );

		ci::fs::path appPath = ci::app::getAppPath();
#ifdef CINDER_MAC
		appPath = appPath.parent_path();
#endif
		boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
		string timestamp = boost::posix_time::to_iso_string( now );

		ci::Vec2i size;
		if ( mSource == SOURCE_CAPTURE )
		{
			size = mCaptures[ mCurrentCapture ]->getSize();
		}
		else // SOURCE_CAPTURE1394
		{
#ifdef CAPTURE_1394
			size = mCapture1394PParams->getCurrentCaptureRef()->getSize();
#endif
		}

		mMovieWriter = ci::qtime::MovieWriter( appPath /
				ci::fs::path( "capture-" + timestamp + ".mov" ),
				size.x, size.y, format );
	}
	mSavingVideo = !mSavingVideo;
}

void CaptureSource::playVideoCB()
{
	ci::fs::path appPath( ci::app::getAppPath() );
#ifdef CINDER_MAC
	appPath = appPath.parent_path();
#endif
	ci::fs::path moviePath = ci::app::getOpenFilePath( appPath );

	if ( !moviePath.empty() )
	{
		try
		{
			mMovie = ci::qtime::MovieSurface( moviePath );
			mMovie.setLoop();
			mMovie.play();
		}
		catch ( ... )
		{
			ci::app::console() << "Unable to load movie " << moviePath << endl;
			mMovie.reset();
		}
	}
}

void CaptureSource::drawParams()
{
	mParams.draw();
	mCaptureSelection.draw();
#ifdef CAPTURE_1394
	mCapture1394PParams->drawParams();
#endif
}

} // namespace mndl

