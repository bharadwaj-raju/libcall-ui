/*
 * Copyright (C) 2026 Bharadwaj Raju <bharadwaj.raju@machinesoul.in>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Author: Bharadwaj Raju <bharadwaj.raju@machinesoul.in>
 */

#include "cui-config.h"

#include "cui-audio-handler-wireplumber.h"
#include "cui-audio-handler.h"
#include "cui-enums.h"

#include <wireplumber-0.5/wp/wp.h>

struct _CuiAudioHandlerWireplumber
{
  GObject parent_instance;
};

typedef struct
{
  WpCore *wp_core;
} CuiAudioHandlerWireplumberPrivate;

struct Context
{
  WpCore *wp_core;
  gboolean want;
};

static void
on_mixer_api_loaded (GObject *source_object, GAsyncResult *res, gpointer data)
{
  struct Context *ctx = (struct Context *)data;
  (void)source_object;

  g_autoptr (GError) error = NULL;
  if (!wp_core_load_component_finish (ctx->wp_core, res, &error))
    {
      g_warning ("Could not load mixer-api plugin for WirePlumber: %s",
                 error->message);
    }
}

static gboolean
is_device_voice_call_capable (WpPipewireObject *device)
{
  g_autoptr (WpIterator) iter
      = wp_pipewire_object_enum_params_sync (device, "EnumProfile", NULL);
  if (!iter)
    {
      return FALSE;
    }
  g_auto (GValue) val = G_VALUE_INIT;

  for (; wp_iterator_next (iter, &val); g_value_unset (&val))
    {
      WpSpaPod *pod = g_value_get_boxed (&val);
      g_autoptr (WpSpaPodParser) parser
          = wp_spa_pod_parser_new_object (pod, NULL);

      gint _index = 0;
      const gchar *name = NULL;

      if (wp_spa_pod_parser_get (parser, "index", "i", &_index, "name", "s",
                                 &name, NULL))
        {
          if (g_strstr_len (name, -1, "Voice Call") != NULL
              || g_strstr_len (name, -1, "VoiceCall") != NULL)
            {
              return TRUE;
            }
        }
    }

  return FALSE;
}

static void
on_node_manager_installed_mute_mic (WpObjectManager *node_mgr, gpointer data)
{
  struct Context *ctx = (struct Context *)data;
  WpCore *core = ctx->wp_core;
  gboolean want_mute = ctx->want;

  WpIterator *obj_iter = wp_object_manager_new_iterator (node_mgr);
  g_auto (GValue) val = G_VALUE_INIT;

  for (; wp_iterator_next (obj_iter, &val); g_value_unset (&val))
    {
      WpPipewireObject *node = g_value_get_object (&val);
      g_autoptr (WpProperties) props = wp_pipewire_object_get_properties (node);
      guint32 node_id = wp_proxy_get_bound_id (WP_PROXY (node));

      g_autoptr (WpPlugin) mixer = wp_plugin_find (core, "mixer-api");
      if (!mixer)
        {
          return;
        }

      GVariantBuilder b;
      g_variant_builder_init (&b, G_VARIANT_TYPE_VARDICT);
      g_variant_builder_add (&b, "{sv}", "mute",
                             g_variant_new_boolean (want_mute));
      GVariant *volume_dict = g_variant_builder_end (&b);

      gboolean success = FALSE;
      g_signal_emit_by_name (mixer, "set-volume", node_id, volume_dict,
                             &success);
      if (!success)
        {
          g_warning ("Could not mute source %s\n",
                     wp_properties_get (props, "node.name"));
        }
    }
}

static void
on_object_manager_installed_mute_mic (WpObjectManager *obj_mgr, gpointer data)
{
  struct Context *ctx = (struct Context *)data;

  g_autoptr (WpIterator) obj_iter = wp_object_manager_new_iterator (obj_mgr);
  g_auto (GValue) val = G_VALUE_INIT;

  for (; wp_iterator_next (obj_iter, &val); g_value_unset (&val))
    {
      WpPipewireObject *device = g_value_get_object (&val);
      if (!is_device_voice_call_capable (device))
        {
          continue;
        }

      g_autoptr (WpProperties) props
          = wp_pipewire_object_get_properties (device);
      const gchar *name = wp_properties_get (props, "device.name");
      const gchar *desc = wp_properties_get (props, "device.description");

      const gchar *device_id = wp_properties_get (props, "object.id");

      WpObjectManager *node_mgr = wp_object_manager_new ();
      WpObjectInterest *source_interest = wp_object_interest_new (
          WP_TYPE_NODE, WP_CONSTRAINT_TYPE_PW_PROPERTY, "device.id", "=s",
          device_id, WP_CONSTRAINT_TYPE_PW_PROPERTY, "media.class", "=s",
          "Audio/Source", NULL);
      wp_object_manager_add_interest_full (node_mgr, source_interest);

      g_signal_connect (node_mgr, "installed",
                        G_CALLBACK (on_node_manager_installed_mute_mic), ctx);
      wp_core_install_object_manager (ctx->wp_core, node_mgr);
      wp_object_manager_request_object_features (node_mgr, WP_TYPE_NODE,
                                                 WP_OBJECT_FEATURES_ALL);
    }
}

static void
on_object_manager_installed_enable_speaker (WpObjectManager *obj_mgr,
                                            gpointer data)
{
  struct Context *ctx = (struct Context *)data;

  g_autoptr (WpIterator) obj_iter = wp_object_manager_new_iterator (obj_mgr);
  g_auto (GValue) val = G_VALUE_INIT;

  for (; wp_iterator_next (obj_iter, &val); g_value_unset (&val))
    {
      WpPipewireObject *device = g_value_get_object (&val);
      g_autoptr (WpIterator) profiles_iter
          = wp_pipewire_object_enum_params_sync (device, "EnumProfile", NULL);
      if (!profiles_iter)
        {
          continue;
        }
      g_auto (GValue) val = G_VALUE_INIT;

      for (; wp_iterator_next (profiles_iter, &val); g_value_unset (&val))
        {
          WpSpaPod *pod = g_value_get_boxed (&val);
          g_autoptr (WpSpaPodParser) parser
              = wp_spa_pod_parser_new_object (pod, NULL);

          gint index = 0;
          const gchar *name = NULL;

          if (wp_spa_pod_parser_get (parser, "index", "i", &index, "name", "s",
                                     &name, NULL))
            {
              if (g_strstr_len (name, -1, "Voice Call") == NULL
                  && g_strstr_len (name, -1, "VoiceCall") == NULL)
                {
                  continue;
                }

              if (g_strstr_len (name, -1, "Speaker") != NULL && ctx->want
                  || g_strstr_len (name, -1, "Earpiece") != NULL && !ctx->want)
                {
                  wp_pipewire_object_set_param (
                      device, "Profile", 0,
                      wp_spa_pod_new_object ("Spa:Pod:Object:Param:Profile",
                                             "Profile", "index", "i", index,
                                             "save", "b", TRUE, NULL));
                }
            }
        }

      g_autoptr (WpProperties) props
          = wp_pipewire_object_get_properties (device);
      const gchar *name = wp_properties_get (props, "device.name");
      const gchar *desc = wp_properties_get (props, "device.description");

      const gchar *device_id = wp_properties_get (props, "object.id");

      WpObjectManager *node_mgr = wp_object_manager_new ();
      WpObjectInterest *source_interest = wp_object_interest_new (
          WP_TYPE_NODE, WP_CONSTRAINT_TYPE_PW_PROPERTY, "device.id", "=s",
          device_id, WP_CONSTRAINT_TYPE_PW_PROPERTY, "media.class", "=s",
          "Audio/Source", NULL);
      wp_object_manager_add_interest_full (node_mgr, source_interest);

      g_signal_connect (node_mgr, "installed",
                        G_CALLBACK (on_node_manager_installed_mute_mic), ctx);
      wp_core_install_object_manager (ctx->wp_core, node_mgr);
      wp_object_manager_request_object_features (node_mgr, WP_TYPE_NODE,
                                                 WP_OBJECT_FEATURES_ALL);
    }
}

static void
cui_audio_handler_wireplumber_iface_init (CuiAudioHandlerInterface *iface);

G_DEFINE_TYPE_WITH_CODE (
    CuiAudioHandlerWireplumber, cui_audio_handler_wireplumber, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (CUI_TYPE_AUDIO_HANDLER,
                           cui_audio_handler_wireplumber_iface_init)
        G_ADD_PRIVATE (CuiAudioHandlerWireplumber))

static void
cui_audio_handler_wireplumber_class_init (
    CuiAudioHandlerWireplumberClass *klass)
{
  // we shouldn't have WP_INIT_SET_GLIB_LOG as it would interfere with
  // consuming applications which set their own GLib log handlers
  // (eg GNOME Calls)
  wp_init (WP_INIT_PIPEWIRE | WP_INIT_SPA_TYPES | WP_INIT_SET_PW_LOG);
}

static void
cui_audio_handler_wireplumber_init (CuiAudioHandlerWireplumber *self)
{
  CuiAudioHandlerWireplumberPrivate *priv
      = cui_audio_handler_wireplumber_get_instance_private (self);
  priv->wp_core = wp_core_new (NULL, NULL, NULL);
  wp_core_connect (priv->wp_core);

  g_info ("Connected to WirePlumber");

  wp_core_load_component (priv->wp_core, "libwireplumber-module-mixer-api",
                          "module", NULL, "mixer-api", NULL,
                          (GAsyncReadyCallback)on_mixer_api_loaded, self);
}

static void
cui_audio_handler_wireplumber_mute_mic (CuiAudioHandler *self,
                                        gboolean want_mute)
{
  CuiAudioHandlerWireplumberPrivate *priv
      = cui_audio_handler_wireplumber_get_instance_private (
          CUI_AUDIO_HANDLER_WIREPLUMBER (self));
  WpObjectManager *obj_mgr = wp_object_manager_new ();
  WpObjectInterest *interest = wp_object_interest_new_type (WP_TYPE_DEVICE);
  wp_object_manager_add_interest_full (obj_mgr, interest);
  wp_object_manager_request_object_features (obj_mgr, WP_TYPE_DEVICE,
                                             WP_OBJECT_FEATURES_ALL);

  struct Context *ctx = (struct Context *)g_malloc (sizeof (struct Context));
  ctx->wp_core = priv->wp_core;
  ctx->want = want_mute;

  g_signal_connect (obj_mgr, "installed",
                    G_CALLBACK (on_object_manager_installed_mute_mic), ctx);
  wp_core_install_object_manager (priv->wp_core, obj_mgr);
}

static void
cui_audio_handler_wireplumber_enable_speaker (CuiAudioHandler *self,
                                              gboolean want_speaker)
{

  CuiAudioHandlerWireplumberPrivate *priv
      = cui_audio_handler_wireplumber_get_instance_private (
          CUI_AUDIO_HANDLER_WIREPLUMBER (self));
  WpObjectManager *obj_mgr = wp_object_manager_new ();
  WpObjectInterest *interest = wp_object_interest_new_type (WP_TYPE_DEVICE);
  wp_object_manager_add_interest_full (obj_mgr, interest);
  wp_object_manager_request_object_features (obj_mgr, WP_TYPE_DEVICE,
                                             WP_OBJECT_FEATURES_ALL);

  struct Context *ctx = (struct Context *)g_malloc (sizeof (struct Context));
  ctx->wp_core = priv->wp_core;
  ctx->want = want_speaker;

  g_signal_connect (obj_mgr, "installed",
                    G_CALLBACK (on_object_manager_installed_enable_speaker),
                    ctx);
  wp_core_install_object_manager (priv->wp_core, obj_mgr);
}

static void
cui_audio_handler_wireplumber_enable_call_mode (CuiAudioHandler *self,
                                                gboolean want_call_mode)
{
  /* no-op: WirePlumber handles this automatically */
}

static void
cui_audio_handler_wireplumber_iface_init (CuiAudioHandlerInterface *iface)
{
  iface->mute_mic = cui_audio_handler_wireplumber_mute_mic;
  iface->enable_speaker = cui_audio_handler_wireplumber_enable_speaker;
  iface->enable_call_mode = cui_audio_handler_wireplumber_enable_call_mode;
}

CuiAudioHandlerWireplumber *
cui_audio_handler_wireplumber_new (void)
{
  return g_object_new (CUI_TYPE_AUDIO_HANDLER_WIREPLUMBER, NULL);
}
