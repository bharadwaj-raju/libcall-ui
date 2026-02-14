/*
 * Copyright (C) 2026 Bharadwaj Raju <bharadwaj.raju@machinesoul.in>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Bharadwaj Raju <bharadwaj.raju@machinesoul.in>
 */

#include "cui-config.h"

#include "cui-audio-handler.h"
#include "cui-enums.h"

#include <gio/gio.h>

G_DEFINE_INTERFACE (CuiAudioHandler, cui_audio_handler, G_TYPE_OBJECT)

void
cui_audio_handler_mute_mic(CuiAudioHandler *self, gboolean want_mute)
{
  CuiAudioHandlerInterface *iface;

  g_return_if_fail (CUI_IS_AUDIO_HANDLER (self));

  iface = CUI_AUDIO_HANDLER_GET_IFACE (self);
  g_return_if_fail (iface->mute_mic);

  iface->mute_mic (self, want_mute);
}


void
cui_audio_handler_enable_speaker(CuiAudioHandler *self, gboolean want_speaker)
{
  CuiAudioHandlerInterface *iface;

  g_return_if_fail (CUI_IS_AUDIO_HANDLER (self));

  iface = CUI_AUDIO_HANDLER_GET_IFACE (self);
  g_return_if_fail (iface->enable_speaker);

  iface->enable_speaker (self, want_speaker);
}


void
cui_audio_handler_select_mode(CuiAudioHandler *self, int mode)
{
  CuiAudioHandlerInterface *iface;

  g_return_if_fail (CUI_IS_AUDIO_HANDLER (self));

  iface = CUI_AUDIO_HANDLER_GET_IFACE (self);
  g_return_if_fail (iface->select_mode);

  iface->select_mode (self, mode);
}

