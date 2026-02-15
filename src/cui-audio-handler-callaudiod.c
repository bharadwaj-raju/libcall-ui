/*
 * Copyright (C) 2026 Bharadwaj Raju <bharadwaj.raju@machinesoul.in>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Bharadwaj Raju <bharadwaj.raju@machinesoul.in>
 */

#include "cui-config.h"

#include "cui-audio-handler-callaudiod.h"
#include "cui-audio-handler.h"
#include "cui-enums.h"

#include <libcallaudio.h>

/* Just print an error, the main point is that libcallaudio uses async DBus calls */
static void
on_libcallaudio_async_finished (gboolean success, GError *error, gpointer data)
{
  if (!success) {
    g_return_if_fail (error && error->message);
    g_warning ("Failed to select audio mode: %s", error->message);
    g_error_free (error);
  }
}

struct _CuiAudioHandlerCallaudiod {
  GObject parent_instance;
};

CuiAudioHandlerCallaudiod *
cui_audio_handler_callaudiod_new (void)
{
  return g_object_new (CUI_TYPE_AUDIO_HANDLER_CALLAUDIOD, NULL);
}


static void
cui_audio_handler_callaudiod_iface_init (CuiAudioHandlerInterface *iface);

G_DEFINE_TYPE_WITH_CODE (CuiAudioHandlerCallaudiod, cui_audio_handler_callaudiod, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CUI_TYPE_AUDIO_HANDLER,
                                                cui_audio_handler_callaudiod_iface_init))

static void
cui_audio_handler_callaudiod_init (CuiAudioHandlerCallaudiod *self)
{
}

static void
cui_audio_handler_callaudiod_class_init (CuiAudioHandlerCallaudiodClass *klass)
{
}

static void
cui_audio_handler_callaudiod_mute_mic (CuiAudioHandler *self, gboolean want_mute)
{
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  call_audio_mute_mic_async (want_mute, on_libcallaudio_async_finished, NULL);
G_GNUC_END_IGNORE_DEPRECATIONS

}

static void
cui_audio_handler_callaudiod_enable_speaker (CuiAudioHandler *self, gboolean want_speaker)
{
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  call_audio_enable_speaker_async (want_speaker, on_libcallaudio_async_finished, NULL);
G_GNUC_END_IGNORE_DEPRECATIONS

}

static void
cui_audio_handler_callaudiod_select_mode (CuiAudioHandler *self, int mode)
{
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    /* TODO Only switch to "call" audio mode for cellular calls */
    call_audio_select_mode_async (mode,
                                  on_libcallaudio_async_finished,
                                  NULL);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
}

static void
cui_audio_handler_callaudiod_iface_init (CuiAudioHandlerInterface *iface)
{
  iface->mute_mic = cui_audio_handler_callaudiod_mute_mic;
  iface->enable_speaker = cui_audio_handler_callaudiod_enable_speaker;
  iface->select_mode = cui_audio_handler_callaudiod_select_mode;
}
