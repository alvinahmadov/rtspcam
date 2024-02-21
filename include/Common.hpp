/**
 * @file Common.hpp
 * @author Alvin Ahmadov <alvin.dev.ahmadov@gmail.com>
 * @date 16.02.24
 * */

#ifndef RTSPCAM_COMMON_HPP
#define RTSPCAM_COMMON_HPP

#include <memory>
#include <string>
#include <optional>

#include <arv.h>
#include <gst/gst.h>
#include <gst/app/app.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/rtsp-server/rtsp-client.h>

/**
 * Shared options for both server and device handles
 * */
struct Options
{
	std::string uvUsbMode{ "async" };
	/// RTSP Server host address
	std::string address{ "0.0.0.0" };
	/// RTSP Server port
	std::string port{ "554" };
	/// RTSP Server path for url
	std::string path{ "stream" };
	/// Default username to authorization
	std::string username{};
	/// Default password to authorization
	std::string password{};

	/// Camera frame width
	int32_t width{ 2448 };
	/// Camera frame height
	int32_t height{ 2048 };
	/// Encoder bitrate
	int64_t bitrate{ 10'000 };
	/// Frame rate
	std::optional<double> frameRate{};
	/// Exposure time for camera device
	std::optional<double> exposure{};
	/// Gain value for camera device
	std::optional<double> gain{};
	int32_t usbMode{ 1 };
	/// Use GPU pipeline url or CPU
	bool useGpu{ true };
};

struct DeviceBounds
{
	double exposureMin;
	double exposureMax;
	double frameRateMax;
	double frameRateMin;
	double gainMax;
	double gainMin;
};

#endif // RTSPCAM_COMMON_HPP
