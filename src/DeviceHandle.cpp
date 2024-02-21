#include "DeviceHandle.hpp"
#include "Callback.hpp"

DeviceHandle::DeviceHandle(const Options *options, uint32_t numStreamBuffers):
	_options{ options },
	_isInitialized{},
	_numDevices{},
	_numClient{},
	_numStreamBuffers{ numStreamBuffers },
	_bounds{},
	_camera{},
	_stream{},
	_state{ GstState::GST_STATE_NULL },
	_source{}
{
	arv_update_device_list();
	_numDevices = arv_get_n_devices();

	if(_numDevices > 0)
	{
		auto deviceId = arv_get_device_id(_numDevices - 1 ? _numDevices > 0 : 0);

		_camera = arv_camera_new(nullptr, nullptr);

		if(!ARV_IS_CAMERA(_camera))
		{
			_isInitialized = false;
			return;
		}

		if(arv_camera_is_uv_device(_camera))
			arv_camera_uv_set_usb_mode(_camera, static_cast<ArvUvUsbMode>(_options->usbMode));
		arv_camera_set_chunk_mode(_camera, false, nullptr);
		arv_camera_set_region(_camera, 0, 0, _options->width, _options->height, nullptr);
		arv_camera_set_exposure_time_auto(_camera, ARV_AUTO_CONTINUOUS, nullptr);
		arv_camera_get_exposure_time_bounds(_camera, &_bounds.exposureMin, &_bounds.exposureMax, nullptr);
		arv_camera_get_frame_rate_bounds(_camera, &_bounds.frameRateMin, &_bounds.frameRateMax, nullptr);
		arv_camera_get_gain_bounds(_camera, &_bounds.gainMin, &_bounds.gainMax, nullptr);
		_isInitialized = true;

		GST_INFO("found camera(s): %d (%s)", _numDevices, deviceId);
	}
	else
	{
		GST_ERROR("no device found!");
	}
}

DeviceHandle::~DeviceHandle()
{
	if(GST_IS_APP_SRC(_source))
	{
		gst_object_unref(_source);
	}
}

bool DeviceHandle::isPlaying() const
{
	return _state == GstState::GST_STATE_PLAYING;
}

void DeviceHandle::setSource(GstAppSrc *source)
{
	if(GST_IS_APP_SRC(source))
	{
		if(_source != nullptr)
		{
			if(isPlaying())
				stopAcquisition();
			gst_object_unref(_source);
		}

		_source = source;

		ArvPixelFormat pixelFormat = arv_camera_get_pixel_format(_camera, nullptr);
		auto capsString = arv_pixel_format_to_gst_caps_string(pixelFormat);
		auto pixelFormatString = arv_camera_get_pixel_format_as_string(_camera, nullptr);

		if(capsString == nullptr)
		{
			GST_ERROR("GStreamer cannot understand this camera pixel format: %s!", pixelFormatString);
			stopAcquisition();
			return;
		}

		GstCaps *caps = gst_caps_from_string(capsString);
		gst_caps_set_simple(caps, "width", G_TYPE_INT, _options->width, "height", G_TYPE_INT, _options->height, "framerate",
												GST_TYPE_FRACTION, 0, 1, nullptr);
		gst_app_src_set_caps(_source, caps);
		gst_caps_unref(caps);
		g_object_set(G_OBJECT(_source), "format", GST_FORMAT_TIME, "is-live", TRUE, "do-timestamp", TRUE, nullptr);
	}
}

void DeviceHandle::startAcquisition()
{
	if(!_isInitialized)
	{
		GST_ERROR("device handle not initialized properly");
		return;
	}

	if(isPlaying())
	{
		GST_INFO("camera is already playing");
		return;
	}

	_stream = arv_camera_create_stream(_camera, cameraStream, nullptr, nullptr);
	if(!ARV_IS_STREAM(_stream))
	{
		GST_ERROR("can not start stream");
		return;
	}

	arv_stream_set_emit_signals(_stream, true);
	arv_stream_create_buffers(_stream, _numStreamBuffers, nullptr, nullptr, nullptr);

	GST_INFO("starting acquisition");
	arv_camera_set_acquisition_mode(_camera, ARV_ACQUISITION_MODE_CONTINUOUS, nullptr);

	if(_options->exposure)
	{
		if(double exposureTime{ _options->exposure.value() }; _bounds.exposureMin <= exposureTime <= _bounds.exposureMax)
		{
			arv_camera_set_exposure_time_auto(_camera, ARV_AUTO_OFF, nullptr);
			arv_camera_set_exposure_time(_camera, exposureTime, nullptr);
		}
	}
	if(_options->frameRate)
	{
		if(double frameRate{ _options->frameRate.value() }; _bounds.frameRateMin <= frameRate <= _bounds.frameRateMax)
		{
			arv_camera_set_frame_rate(_camera, frameRate, nullptr);
		}
	}
	if(_options->gain)
	{
		if(double gain{ _options->gain.value() }; _bounds.gainMin <= gain <= _bounds.gainMax)
		{
			arv_camera_set_gain(_camera, gain, nullptr);
		}
	}

	arv_camera_start_acquisition(_camera, nullptr);

	_state = GstState::GST_STATE_PLAYING;
	g_signal_connect(_stream, "new-buffer", reinterpret_cast<GCallback>(newBuffer), _source);
}

void DeviceHandle::stopAcquisition()
{
	if(!_isInitialized)
	{
		GST_ERROR("device handle not initialized properly");
		return;
	}

	GST_INFO("stopping acquisition");

	if(ARV_IS_STREAM(_stream))
	{
		arv_stream_set_emit_signals(_stream, false);
		g_object_unref(_stream);
		_stream = nullptr;
	}

	if(ARV_IS_CAMERA(_camera))
		arv_camera_stop_acquisition(_camera, nullptr);

	if(GST_IS_APP_SRC(_source))
	{
		gst_object_unref(_source);
		_source = nullptr;
	}
	_state = GstState::GST_STATE_NULL;
}

int32_t DeviceHandle::incrNumClient()
{
	return ++_numClient;
}

int32_t DeviceHandle::decrNumClient()
{
	return _numClient == 0 ? _numClient : --_numClient;
}