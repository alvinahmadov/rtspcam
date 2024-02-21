/**
* @file ServerHandle.hpp
* @author Alvin Ahmadov <alvin.dev.ahmadov@gmail.com>*
* @date 16.02.24
* */

#ifndef RTSPCAM_SERVERHANDLE_HPP
#define RTSPCAM_SERVERHANDLE_HPP

#include "DeviceHandle.hpp"

/**
 * @brief Handles internal structure of RTSP Server.
 *
 * The class initializes GStreamer RTSP server, handles authorization
 * and callbacks of the server.
 * It also initializes device handle to capture frames from IP Camera
 * and push them to the pipeline of the server.
 *
 * @author Alvin Ahmadov <alvin.dev.ahmadov@gmail.com>
 * @ingroup Server
 * */
class ServerHandle
{
public:
	explicit ServerHandle(const Options *options);
	~ServerHandle();

	/**
	 * @brief Add username with username and password to the authorized users.
	 *
	 * @param username[in] User name to authorize
	 * @param pass[in] User password to add
	 * */
	void addUser(std::string_view username, std::string_view password) noexcept;

	/**
	 * @brief Attach server to the main context.
	 *
	 * @param[in] timeoutInterval Timeout for seession cleaning in seconds.
	 * */
	void attach(uint32_t timeoutInterval = 2);

protected:
	/**
	 * @brief Intialize media factory.
	 *
	 * The media factory is responsible for creating or recycling
	 * media objects based on the passed URL.
	 * In this method we initialize media factory with predefined launch
	 * string as url parameter.
	 * Also we bind media factory and media related callbacks here.
	 *
	 * @param path URL path
	 * */
	void initMediaFactory(const char *path = "/live") noexcept;

	/**
	 * @brief Initialize GStreamer RTSP Server authentication logic.
	 *
	 * The GstRTSPAuth object is responsible for checking if the current
	 * user is allowed to perform requested actions.
	 * @see https://gstreamer.freedesktop.org/documentation/gst-rtsp-server/rtsp-auth.html?gi-language=c
	 *
	 * */
	void initAuth() noexcept;

private:
	bool _enableAuth;
	const Options *_options;
	GstRTSPServer *_server;
	GstRTSPMediaFactory *_factory;
	GstRTSPAuth *_auth;
	DeviceHandle *_deviceHandle;
};

#endif // RTSPCAM_SERVERHANDLE_HPP
