/**
* @file DeviceHandle.hpp
* @author Alvin Ahmadov <alvin.dev.ahmadov@gmail.com>*
* @date 16.02.24
* */

#ifndef RTSPCAM_DEVICEHANDLE_HPP
#define RTSPCAM_DEVICEHANDLE_HPP

#include "Common.hpp"

/**
 * @class DeviceHandle
 *
 * The class handles camera device to capture frames and push
 * buffers to GStreamer app source.
 * */
class DeviceHandle
{
public:
	explicit DeviceHandle(const Options *options, uint32_t numStreamBuffers = 30);
	~DeviceHandle();

	[[nodiscard]]
	bool isPlaying() const;

	/**
	 * @brief Set app source from server media factory.
	 *
	 * @param source App source to use for push of buffers.
	 * */
	void setSource(GstAppSrc *source);

	/**
	 * Starts acquisition of frames from camera device to
	 * push buffers from stream to app source.
	 * */
	void startAcquisition();

	/**
	 * Stops acquisition and frees allocated objects.
	 * */
	void stopAcquisition();

	/**
	 * @brief Increase number of clients.
	 * */
	int32_t incrNumClient();

	/**
	 * @brief Decrease number of clients.
	 * */
	int32_t decrNumClient();

private:
	const Options *_options;
	bool _isInitialized;
	uint32_t _numDevices;
	int32_t _numClient;
	uint32_t _numStreamBuffers;

	GstState _state;
	DeviceBounds _bounds;
	ArvCamera *_camera;
	ArvStream *_stream;
	GstAppSrc *_source;
};

#endif // RTSPCAM_DEVICEHANDLE_HPP
