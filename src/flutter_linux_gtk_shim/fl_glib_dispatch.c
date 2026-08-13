// SPDX-License-Identifier: MIT
//
// GTK plugins written against the real flutter_linux embedder assume a GLib
// main loop is running on the platform thread: g_idle_add() is how they hop
// back onto it to invoke method channels from their own worker threads.
// flutter-drm-embedder runs an sd_event loop instead and never iterates GLib's
// default GMainContext, so without this those idle callbacks are queued and
// never dispatched - the plugin looks alive, but every native->Dart call it
// makes silently disappears.
//
// So the shim pumps the default context from a repeating platform task, which
// keeps the dispatch on the platform thread where plugins expect it.

#include <glib.h>

#include "fl_glib_dispatch_internal.h"
#include "flutter_drm_embedder_shim.h"

// How often the default context is pumped. Sets the worst-case latency for a
// queued idle callback; anything already pending is drained in one go.
#define FL_GLIB_PUMP_INTERVAL_US 20000

// Cap on sources dispatched per pump, so a plugin that re-arms an idle from
// inside its own callback cannot starve the embedder's event loop.
#define FL_GLIB_PUMP_MAX_DISPATCH 64

static int fl_glib_pump(void *userdata) {
    (void) userdata;

    for (int i = 0; i < FL_GLIB_PUMP_MAX_DISPATCH; i++) {
        if (!g_main_context_iteration(NULL, FALSE)) {
            break;
        }
    }

    flutter_drm_embedder_post_platform_task_with_time(
        fl_glib_pump,
        NULL,
        (uint64_t) g_get_monotonic_time() + FL_GLIB_PUMP_INTERVAL_US
    );

    return 0;
}

void fl_glib_dispatch_start(void) {
    static gboolean started = FALSE;

    if (started) {
        return;
    }
    started = TRUE;

    flutter_drm_embedder_post_platform_task(fl_glib_pump, NULL);
}
