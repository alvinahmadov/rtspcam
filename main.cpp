#include <optional>
#include <arv.h>

#include "Common.hpp"
#include "ServerHandle.hpp"

static const std::string gPlugins[] = { "appsrc", "videoconvert" };

bool checkPlugins();

Options parseOptions(int argc, char *argv[]);

int main(int argc, char **argv)
{
	Options options{};
	GMainLoop *mainLoop;

	std::unique_ptr<ServerHandle> serverHandle;

	gst_init(&argc, &argv);
	checkPlugins();

	mainLoop = g_main_loop_new(nullptr, false);
	options = parseOptions(argc, argv);

	serverHandle = std::make_unique<ServerHandle>(&options);
	serverHandle->attach(4);

	g_main_loop_run(mainLoop);
	return 0;
}

bool checkPlugins()
{
	GstRegistry *registry;
	GstPluginFeature *feature;
	bool success{ true };

	registry = gst_registry_get();
	for(const auto &pluginName : gPlugins)
	{
		feature = gst_registry_lookup_feature(registry, pluginName.c_str());
		if(!GST_IS_PLUGIN_FEATURE(feature))
		{
			GST_ERROR("Gstreamer plugin '%s' is missing.\n", pluginName.c_str());
			success = false;
		}
		else
		{
			g_object_unref(feature);
		}
	}

	if(!success)
		GST_ERROR("Check your gstreamer installation.\n");

	return success;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantConditionsOC"
#pragma ide diagnostic ignored "UnreachableCode"
Options parseOptions(int argc, char *argv[])
{
	GOptionContext *context;
	GError *error{};
	Options options{};
	int32_t width{ 2448 }, height{ 2048 };
	int64_t bitrate{ 4096 };
	double *frameRate{};
	double *exposure{};
	double *gain{};
	const char *mode{};
	const char *address{};
	const char *port{};
	const char *streamUri{};
	const char *username{};
	const char *password{};

	static const GOptionEntry optionEntries[] = {
		{ "address", 'a', 0, G_OPTION_ARG_STRING, &address, "RTSP server streaming address", "default: 127.0.0.1" },
		{ "port", 'p', 0, G_OPTION_ARG_STRING, &port, "RTSP server streaming port", "default: 8554" },
		{ "stream-uri", 's', 0, G_OPTION_ARG_NONE, &streamUri, "RTSP server streaming uri streamUri", "default: stream" },
		{ "username", 'U', 0, G_OPTION_ARG_NONE, &username, "RTSP server streaming uri streamUri", "default: stream" },
		{ "password", 'P', 0, G_OPTION_ARG_NONE, &password, "RTSP server streaming uri streamUri", "default: stream" },
		{ "frame-rate", 'f', 0, G_OPTION_ARG_DOUBLE, frameRate, "Acquisition frame rate", "default: 30" },
		{ "exposure", 'e', 0, G_OPTION_ARG_DOUBLE, exposure, "Acquisition exposure time", "default: 15000" },
		{ "gain", 'g', 0, G_OPTION_ARG_DOUBLE, gain, "Acquisition gain value", "default: 22" },
		{ "width", 'w', 0, G_OPTION_ARG_INT, &width, "Region width", "default: 2448" },
		{ "height", 'h', 0, G_OPTION_ARG_INT, &height, "Region height", "default: 2048" },
		{ "bitrate", 'b', 0, G_OPTION_ARG_INT64, &bitrate, "Encoder bitrate", "default: 10000" },
		{ "mode", 'm', 0, G_OPTION_ARG_STRING, &mode, "Hardware to use", "gpu|cpu" },
		{ nullptr }
	};

	context = g_option_context_new(nullptr);
	g_option_context_add_main_entries(context, optionEntries, nullptr);
	g_option_context_add_group(context, gst_init_get_option_group());
	if(!g_option_context_parse(context, &argc, &argv, &error))
	{
		g_option_context_free(context);
		GST_ERROR("Option parsing failed: %s\n", error->message);
		g_error_free(error);
	}

	g_option_context_free(context);

	if(address != nullptr)
		options.address = address;
	if(port != nullptr)
		options.port = port;
	if(streamUri != nullptr)
		options.path = streamUri;
	if(username != nullptr)
		options.username = username;
	if(password != nullptr)
		options.password = password;

	options.usbMode = ARV_UV_USB_MODE_DEFAULT;
	options.width = width;
	options.height = height;
	if(exposure != nullptr)
		options.exposure = *exposure;
	if(frameRate != nullptr)
		options.frameRate = *frameRate;
	if(gain != nullptr)
		options.gain = *gain;
	options.bitrate = bitrate;

	return options;
}
#pragma clang diagnostic pop
