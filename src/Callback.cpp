#include "Callback.hpp"
#include "DeviceHandle.hpp"

bool cleanupTimeout(GstRTSPServer *server)
{
	GstRTSPSessionPool *pool;
	pool = gst_rtsp_server_get_session_pool(server);
	gst_rtsp_session_pool_cleanup(pool);
	g_object_unref(pool);

	return true;
}

void configureMedia([[maybe_unused]] GstRTSPMediaFactory *factory, GstRTSPMedia *media, void *data)
{
	GstBin *bin;
	GstElement *source;
	auto devHandle = reinterpret_cast<DeviceHandle *>(data);

	gst_rtsp_media_set_shared(media, true);
	// get the element used for providing the streams of the media
	bin = reinterpret_cast<GstBin *>(gst_rtsp_media_get_element(media));
	// get our appsrc, we named it 'srvsrc' with the name property
	source = gst_bin_get_by_name_recurse_up(bin, "srvsrc");
	devHandle->setSource(reinterpret_cast<GstAppSrc *>(source));
	devHandle->startAcquisition();
	g_signal_connect(media, "new-state", reinterpret_cast<GCallback>(mediaStateChanged), devHandle);
	gst_object_unref(bin);
}

void mediaConstructed([[maybe_unused]] GstRTSPMediaFactory *factory, GstRTSPMedia *media, [[maybe_unused]] void *data)
{
	uint32_t i, numStreams;

	numStreams = gst_rtsp_media_n_streams(media);

	for(i = 0; i < numStreams; i++)
	{
		GstRTSPAddressPool *pool;
		GstRTSPStream *stream;
		char *min, *max;

		stream = gst_rtsp_media_get_stream(media, i);

		if(stream == nullptr)
			continue;

		// make a new address pool
		pool = gst_rtsp_address_pool_new();

		min = g_strdup_printf("224.3.0.%d", (2 * i) + 1);
		max = g_strdup_printf("224.3.0.%d", (2 * i) + 2);
		gst_rtsp_address_pool_add_range(pool, min, max, 5000 + (10 * i), 5010 + (10 * i), 1);
		g_free(min);
		g_free(max);

		gst_rtsp_stream_set_address_pool(stream, pool);
		g_object_unref(pool);
	}
}

void mediaStateChanged([[maybe_unused]] GstRTSPMedia *media, GstState state, [[maybe_unused]] void *data)
{
	auto devHandle = reinterpret_cast<DeviceHandle *>(data);
	switch(state)
	{
		case GST_STATE_NULL:
			devHandle->stopAcquisition();
			break;
		default:
			break;
	}
}

void clientConnected([[maybe_unused]] GstRTSPServer *server, GstRTSPClient *client, void *data)
{
	auto _devHandle = reinterpret_cast<DeviceHandle *>(data);
	g_message("client connected (current: %d)\n", _devHandle->incrNumClient());
	// hook the client close callback
	g_signal_connect(client, "closed", reinterpret_cast<GCallback>(clientClosed), _devHandle);
}

void clientClosed([[maybe_unused]] GstRTSPClient *client, [[maybe_unused]] void *data)
{
	auto _devHandle = reinterpret_cast<DeviceHandle *>(data);
	g_message("client disconnected (current: %d)\n", _devHandle->decrNumClient());
}

GstBuffer *toGstBuffer(ArvBuffer *arvBuffer, guint partId, ArvStream *stream)
{
	auto releaseData = new ArvGstBufferReleaseData();
	int32_t arvRowStride;
	int32_t width, height;
	uint8_t *data;
	size_t size;
	uint8_t *bufferData;
	size_t bufferSize;

	bufferData = (uint8_t *)arv_buffer_get_part_data(arvBuffer, partId, &bufferSize);
	arv_buffer_get_part_region(arvBuffer, partId, nullptr, nullptr, &width, &height);
	arvRowStride = static_cast<int>(
			(width * ARV_PIXEL_FORMAT_BIT_PER_PIXEL(arv_buffer_get_part_pixel_format(arvBuffer, partId)) / 8));

	g_weak_ref_init(&releaseData->stream, stream);
	releaseData->arvBuffer = arvBuffer;

	// Gstreamer requires row stride to be a multiple of 4
	if((arvRowStride & 0x3) != 0)
	{
		int gstRowStride;
		int i;

		gstRowStride = (arvRowStride & ~(0x3)) + 4;

		size = height * gstRowStride;
		data = new uint8_t[size];

		for(i = 0; i < height; i++)
		{
			memcpy(data + i * gstRowStride, bufferData + i * arvRowStride, arvRowStride);
		}

		releaseData->data = data;
	}
	else
	{
		data = bufferData;
		size = bufferSize;
	}

	return gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY, data, size, 0, size, releaseData,
																		 reinterpret_cast<GDestroyNotify>(gstBufferReleaseCallback));
}

void gstBufferReleaseCallback(ArvGstBufferReleaseData *releaseData)
{
	auto *stream = static_cast<ArvStream *>(g_weak_ref_get(&releaseData->stream));

	delete[] releaseData->data;

	if(ARV_IS_STREAM(stream))
	{
		int32_t nInputBuffers, nOutputBuffers, nBufferFilling;

		arv_stream_get_n_owned_buffers(stream, &nInputBuffers, &nOutputBuffers, &nBufferFilling);
		arv_stream_push_buffer(stream, releaseData->arvBuffer);
		g_object_unref(stream);
	}
	else
	{
		GST_WARNING("invalid stream object...");
		g_object_unref(releaseData->arvBuffer);
	}

	g_weak_ref_clear(&releaseData->stream);
	delete releaseData;
}

void cameraStream([[maybe_unused]] void *data, ArvStreamCallbackType type, [[maybe_unused]] ArvBuffer *buffer)
{
	if(type == ARV_STREAM_CALLBACK_TYPE_INIT)
	{
		if(!arv_make_thread_realtime(10) && !arv_make_thread_high_priority(-10))
		{
			GST_WARNING("failed to make stream thread high priority");
		}
	}
}

void newBuffer(ArvStream *stream, GstAppSrc *source)
{
	int32_t nInputBuffers, nOutputBuffers, nBufferFilling;
	ArvBuffer *arvBuffer = arv_stream_pop_buffer(stream);
	if(arvBuffer == nullptr)
	{
		GST_WARNING("empty camera buffer, return");
		return;
	}

	arv_stream_get_n_owned_buffers(stream, &nInputBuffers, &nOutputBuffers, &nBufferFilling);

	if(arv_buffer_get_status(arvBuffer) == ARV_BUFFER_STATUS_SUCCESS &&
		 nInputBuffers + nOutputBuffers + nBufferFilling > 0)
	{
		size_t size;

		arv_buffer_get_part_data(arvBuffer, 0, &size);
		gst_app_src_push_buffer(source, toGstBuffer(arvBuffer, 0, stream));
	}
	else
	{
		arv_stream_push_buffer(stream, arvBuffer);
	}
}
