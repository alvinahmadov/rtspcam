/**
 * @file Callback.hpp
 * @author Alvin Ahmadov <alvin.dev.ahmadov@gmail.com>
 * @date 16.02.24
 * */

#ifndef RTSPCAM_CALLBACK_HPP
#define RTSPCAM_CALLBACK_HPP

#include "Common.hpp"

struct ArvGstBufferReleaseData
{
	GWeakRef stream;
	ArvBuffer *arvBuffer;
	guint8 *data;
};

/**
 * \brief Timeout callback is periodically run to clean up the expired sessions from the
 * pool.
 * This needs to be run explicitly currently but might be done automatically as part of the mainloop.
 *
 * \param server[in] - A Gstreamer RTSP server instance
 */
bool cleanupTimeout(GstRTSPServer *server);

/**
 * \brief Called when a new media pipeline is constructed.
 *
 * We can query the pipeline and configure our appsrc
 *
 * \param factory a Gstreamer RTSP media factory instance.
 * \param media a media to be configured.
 * \param context app context.
 * */
void configureMedia(GstRTSPMediaFactory *factory, GstRTSPMedia *media, void *data);

void mediaConstructed(GstRTSPMediaFactory *factory, GstRTSPMedia *media, void *data);

void mediaStateChanged(GstRTSPMedia *media, GstState state, void *data);

/**
 * \brief The signal called when new client connected to server.
 * At least 1 client must be connected to consume camera videosink data.
 *
 * \param server gstreamer RTSPServer instance.
 * \param client connected client instance.
 * */
void clientConnected(GstRTSPServer *server, GstRTSPClient *client, void *data);

/**
 * \brief The signal called when a client disconnects from server.
 *
 * We stop feeding data to server when there is no connected client.
 *
 * \param server gstreamer RTSPServer instance.
 * \param client connected client instance.
 * */
void clientClosed(GstRTSPClient *client, void *data);

GstBuffer *toGstBuffer(ArvBuffer *arvBuffer, guint partId, ArvStream *stream);

void gstBufferReleaseCallback(ArvGstBufferReleaseData *releaseData);

void cameraStream(void *data, ArvStreamCallbackType type, ArvBuffer *buffer);

void newBuffer(ArvStream *stream, GstAppSrc *source);

#endif // RTSPCAM_CALLBACK_HPP
