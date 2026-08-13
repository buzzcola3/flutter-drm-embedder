// SPDX-License-Identifier: MIT
#ifndef FL_GLIB_DISPATCH_INTERNAL_H
#define FL_GLIB_DISPATCH_INTERNAL_H

/// Starts dispatching GLib's default main context on the platform thread.
/// Idempotent. See fl_glib_dispatch.c for why plugins need this.
void fl_glib_dispatch_start(void);

#endif  // FL_GLIB_DISPATCH_INTERNAL_H
