#include <fmt/format.h>

#include "ServerHandle.hpp"
#include "Callback.hpp"

static constexpr const char *CPU_LAUNCH_STRING{
	"appsrc name=srvsrc ! "
	"bayer2rgb ! video/x-raw, format=(string)RGBx ! "
	"videoconvert ! video/x-raw, format=(string)I420, width=(int){0}, height=(int){1} ! "
	"queue ! "
	"x264enc tune=zerolatency bitrate={2} ! "
	"video/x-h264, width=(int){0}, height=(int){1}, stream-format=byte-stream, profile=main ! "
	"rtph264pay name=pay0 pt=96"
};

static constexpr const char *GPU_LAUNCH_STRING{
	"appsrc name=srvsrc ! "
	"bayer2rgb ! "
	"nvvidconv ! video/x-raw(memory:NVMM), width=(int){0}, height=(int){1}, format=(string)I420 ! "
	"queue max-size-buffers=300 ! "
	"nvv4l2h264enc bitrate={2} preset-level=2 profile=2 insert-sps-pps=1 ! "
	"rtph264pay name=pay0 pt=96"
};

ServerHandle::ServerHandle(const Options *options):
	_options{ options },
	_factory{},
	_auth{}
{
	auto path = (_options->path.starts_with("/") ? _options->path : "/" + _options->path);
	_deviceHandle = new DeviceHandle(_options);
	_enableAuth = !_options->username.empty() && !_options->password.empty();

	/* create a server instance */
	_server = gst_rtsp_server_new();
	gst_rtsp_server_set_service(_server, _options->port.c_str());
	gst_rtsp_server_set_address(_server, _options->address.c_str());
	initMediaFactory(path.c_str());
	if(_enableAuth)
	{
		initAuth();
		addUser(_options->username, _options->password);
	}
	g_signal_connect(_server, "client-connected", reinterpret_cast<GCallback>(clientConnected), _deviceHandle);
}

ServerHandle::~ServerHandle()
{
	gst_object_unref(_auth);
	gst_object_unref(_factory);
	gst_object_unref(_server);
	delete _deviceHandle;
}

void ServerHandle::addUser(std::string_view username, std::string_view password) noexcept
{
	GstRTSPToken *token;
	char *basic;
	if(_enableAuth)
	{
		token = gst_rtsp_token_new(GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, "username", nullptr);
		basic = gst_rtsp_auth_make_basic(username.data(), password.data());
		gst_rtsp_auth_add_basic(_auth, basic, token);
		g_free(basic);
		gst_rtsp_token_unref(token);
	}
}

void ServerHandle::attach(uint32_t timeoutInterval)
{
	if(gst_rtsp_server_attach(_server, nullptr) == 0)
		throw std::runtime_error("failed attach server to the main loop\n");

	// add a timeout for the session cleanup
	g_timeout_add_seconds(timeoutInterval, reinterpret_cast<GSourceFunc>(cleanupTimeout), _server);

	GST_INFO("Stream ready at rtsp://%s:%s/%s", _options->address.c_str(), _options->port.c_str(),
					 _options->path.c_str());
}

void ServerHandle::initMediaFactory(const char *path) noexcept
{
	// get the mount points for this server, every server has a default object
	// that be used to map uri mount points to media factories
	std::string launchString;

	if(_options->useGpu)
	{
		launchString = fmt::format(GPU_LAUNCH_STRING, _options->width, _options->height, _options->bitrate);
	}
	else
	{
		launchString = fmt::format(CPU_LAUNCH_STRING, _options->width, _options->height, _options->bitrate);
	}
	GstRTSPMountPoints *mountPoints = gst_rtsp_server_get_mount_points(_server);
	_factory = gst_rtsp_media_factory_new();
	gst_rtsp_media_factory_set_launch(_factory, launchString.c_str());
	gst_rtsp_media_factory_set_shared(_factory, true);
	gst_rtsp_mount_points_add_factory(mountPoints, path, _factory);
	// notify when our media is ready, This is called whenever someone asks for
	// the media and a new pipeline with our appsrc is created
	g_signal_connect(_factory, "media-configure", reinterpret_cast<GCallback>(configureMedia), _deviceHandle);
	g_signal_connect(_factory, "media-constructed", reinterpret_cast<GCallback>(mediaConstructed), nullptr);
	g_object_unref(mountPoints);
}

void ServerHandle::initAuth() noexcept
{
	if(_auth != nullptr)
		return;

	GstRTSPPermissions *permissions;
	// make a new authentication manager. it can be added to control access to all
	// the factories on the server or on individual factories.
	_auth = gst_rtsp_auth_new();
	// configure in the server
	gst_rtsp_server_set_auth(_server, _auth);
	// add permissions for the user media role
	permissions = gst_rtsp_permissions_new();
	gst_rtsp_permissions_add_role(permissions, "user", GST_RTSP_PERM_MEDIA_FACTORY_ACCESS, G_TYPE_BOOLEAN, true,
																GST_RTSP_PERM_MEDIA_FACTORY_CONSTRUCT, G_TYPE_BOOLEAN, true, nullptr);
	gst_rtsp_media_factory_set_permissions(_factory, permissions);
	gst_rtsp_permissions_unref(permissions);
}
