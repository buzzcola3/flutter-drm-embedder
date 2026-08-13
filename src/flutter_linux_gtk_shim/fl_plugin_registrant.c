// SPDX-License-Identifier: MIT
#include "flutter_linux/fl_plugin_registry.h"

#include <stdio.h>

#include "config.h"
#include "fl_glib_dispatch_internal.h"
#include "fl_plugin_registrar_internal.h"
#include "flutter_drm_embedder_shim.h"
#include "plugin_loader.h"

__attribute__((constructor))
static void fl_gtk_shim_init(void) {
    fprintf(stderr, "libflutter_linux_gtk.so version %s loaded\n", FLUTTER_DRM_EMBEDDER_VERSION);
}

__attribute__((weak)) void fl_register_plugins(FlPluginRegistry *registry);

void flutter_drm_embedder_register_gtk_plugins(struct flutter_drm_embedder *flutter_drm_embedder) {
    fprintf(stderr, "[plugin_registrant] registering GTK plugins for embedder %p\n", (void *)flutter_drm_embedder);

    struct gtk_plugin_loader *loader = gtk_plugin_loader_load(flutter_drm_embedder);
    if (loader != NULL) {
        fprintf(stderr, "[plugin_registrant] plugin_loader loaded plugins from bundle dir\n");
        flutter_drm_embedder_set_gtk_plugin_loader(flutter_drm_embedder, loader);
        // Plugins deliver native->Dart calls via g_idle_add(), which nothing
        // would otherwise dispatch here.
        fl_glib_dispatch_start();
    } else {
        fprintf(stderr, "[plugin_registrant] plugin_loader found no plugins in bundle dir\n");
    }

    if (fl_register_plugins == NULL) {
        fprintf(stderr, "[plugin_registrant] fl_register_plugins weak symbol is NULL, skipping static registration\n");
        return;
    }

    fl_glib_dispatch_start();

    fprintf(stderr, "[plugin_registrant] calling fl_register_plugins (static registration)\n");
    FlPluginRegistrar *registrar = fl_plugin_registrar_new_for_flutter_drm_embedder(flutter_drm_embedder);
    if (registrar == NULL) {
        fprintf(stderr, "[plugin_registrant] failed to create plugin registrar\n");
        return;
    }

    fl_register_plugins((FlPluginRegistry *) registrar);
    fprintf(stderr, "[plugin_registrant] static registration complete, unreffing registrar %p\n", (void *)registrar);
    g_object_unref(registrar);
}
