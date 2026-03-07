/*
 * Copyright (C) 2026 Bharadwaj Raju <bharadwaj.raju@machinesoul.in>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Bharadwaj Raju <bharadwaj.raju@machinesoul.in>
 */

#pragma once

#include "cui-config.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define CUI_TYPE_AUDIO_HANDLER_WIREPLUMBER                                     \
  (cui_audio_handler_wireplumber_get_type ())

G_DECLARE_FINAL_TYPE (CuiAudioHandlerWireplumber, cui_audio_handler_wireplumber,
                      CUI, AUDIO_HANDLER_WIREPLUMBER, GObject)

CuiAudioHandlerWireplumber *cui_audio_handler_wireplumber_new (void);

G_END_DECLS
