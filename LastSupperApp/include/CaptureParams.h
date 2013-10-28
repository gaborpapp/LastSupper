#pragma once

#include "cinder/Capture.h"

#include "mndlkit/params/PParams.h"

namespace mndl {

typedef std::shared_ptr< class CaptureParams > CaptureParamsRef;

class CaptureParams : public ci::Capture
{
protected:
	CaptureParams();
	CaptureParams( int32_t width, int32_t height, const ci::Capture::DeviceRef device = ci::Capture::DeviceRef());

public:
	static CaptureParamsRef create( int32_t width, int32_t height, const ci::Capture::DeviceRef device = ci::Capture::DeviceRef() ) { return CaptureParamsRef( new CaptureParams( width, height, device ) ); }
	static void setup();

	void buildParams();
	void updateParams();

	static void removeParams();

private:
#ifdef CINDER_MSW
	void buildParam( ci::Capture::Device::SettingsFilterType settingsType, int *settingsValue, const std::string &name );
	void updateParam( ci::Capture::Device::SettingsFilterType settingsType, int *settingsValue, int *settingsValueLast );
#endif

	static std::string getMinMaxStepString( int min, int max, int step );

private:
	// params
	static mndl::params::PInterfaceGl mParams;

	static int                        mBrightness;
	static int                        mContrast;
	static int                        mSharpness;
	static int                        mGamma;
	static int                        mBacklightCompensation;
	static int                        mGain;
};

} // namspace mndl
