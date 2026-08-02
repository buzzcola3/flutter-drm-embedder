// SPDX-License-Identifier: MIT
#ifndef FLUTTER_DRM_EMBEDDER_SHIM_H
#define FLUTTER_DRM_EMBEDDER_SHIM_H

#include <stddef.h>
#include <stdint.h>

#include <flutter_embedder.h>

struct flutter_drm_embedder;
struct texture;
struct gtk_plugin_loader;

struct plugin_registry;
struct texture_registry;

struct plugin_registry *flutter_drm_embedder_get_plugin_registry(struct flutter_drm_embedder *flutter_drm_embedder);
int flutter_drm_embedder_send_platform_message(struct flutter_drm_embedder *flutter_drm_embedder,
                                    const char *channel,
                                    const uint8_t *message,
                                    size_t message_size,
                                    FlutterPlatformMessageResponseHandle *responsehandle);
int flutter_drm_embedder_respond_to_platform_message(const FlutterPlatformMessageResponseHandle *handle,
                                          const uint8_t *message,
                                          size_t message_size);

FlutterPlatformMessageResponseHandle *flutter_drm_embedder_create_platform_message_response_handle(struct flutter_drm_embedder *flutter_drm_embedder,
                                                                                       FlutterDataCallback data_callback,
                                                                                       void *userdata);
void flutter_drm_embedder_release_platform_message_response_handle(struct flutter_drm_embedder *flutter_drm_embedder,
                                                        FlutterPlatformMessageResponseHandle *handle);

struct texture *flutter_drm_embedder_create_texture(struct flutter_drm_embedder *flutter_drm_embedder);

void flutter_drm_embedder_set_gtk_plugin_loader(struct flutter_drm_embedder *flutter_drm_embedder, struct gtk_plugin_loader *loader);
struct gtk_plugin_loader *flutter_drm_embedder_get_gtk_plugin_loader(struct flutter_drm_embedder *flutter_drm_embedder);

void flutter_drm_embedder_set_fl_texture_registrar(struct flutter_drm_embedder *flutter_drm_embedder, void *registrar);
void *flutter_drm_embedder_get_fl_texture_registrar(struct flutter_drm_embedder *flutter_drm_embedder);

int flutter_drm_embedder_post_platform_task(int (*callback)(void *userdata), void *userdata);
int flutter_drm_embedder_post_platform_task_with_time(int (*callback)(void *userdata),
                                                     void *userdata,
                                                     uint64_t target_time_usec);

/// Starts dispatching GLib's default main context on the platform thread.
/// Idempotent. See fl_glib_dispatch.c for why plugins need this.
void fl_glib_dispatch_start(void);

#endif  // FLUTTER_DRM_EMBEDDER_SHIM_H
