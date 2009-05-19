/*
 * librest - RESTful web services access
 * Copyright (c) 2008, 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
 *          Ross Burton <ross@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <config.h>
#include <string.h>
#include <rest/rest-proxy.h>
#include <libsoup/soup.h>
#include "flickr-proxy.h"
#include "flickr-proxy-private.h"
#include "flickr-proxy-call.h"

G_DEFINE_TYPE (FlickrProxy, flickr_proxy, REST_TYPE_PROXY)

enum {
  PROP_0,
  PROP_CONSUMER_KEY,
  PROP_CONSUMER_SECRET,
  PROP_TOKEN,
};

static RestProxyCall *
_new_call (RestProxy *proxy)
{
  RestProxyCall *call;

  call = g_object_new (FLICKR_TYPE_PROXY_CALL,
                       "proxy", proxy,
                       NULL);

  return call;
}

static void
flickr_proxy_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  FlickrProxyPrivate *priv = PROXY_GET_PRIVATE (object);

  switch (property_id) {
  case PROP_CONSUMER_KEY:
    g_value_set_string (value, priv->consumer_key);
    break;
  case PROP_CONSUMER_SECRET:
    g_value_set_string (value, priv->consumer_secret);
    break;
  case PROP_TOKEN:
    g_value_set_string (value, priv->token);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
flickr_proxy_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  FlickrProxyPrivate *priv = PROXY_GET_PRIVATE (object);

  switch (property_id) {
  case PROP_CONSUMER_KEY:
    if (priv->consumer_key)
      g_free (priv->consumer_key);
    priv->consumer_key = g_value_dup_string (value);
    break;
  case PROP_CONSUMER_SECRET:
    if (priv->consumer_secret)
      g_free (priv->consumer_secret);
    priv->consumer_secret = g_value_dup_string (value);
    break;
  case PROP_TOKEN:
    if (priv->token)
      g_free (priv->token);
    priv->token = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
flickr_proxy_finalize (GObject *object)
{
  FlickrProxyPrivate *priv = PROXY_GET_PRIVATE (object);

  g_free (priv->consumer_key);
  g_free (priv->consumer_secret);
  g_free (priv->token);

  G_OBJECT_CLASS (flickr_proxy_parent_class)->finalize (object);
}

#ifndef G_PARAM_STATIC_STRINGS
#define G_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#endif

static void
flickr_proxy_class_init (FlickrProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  RestProxyClass *proxy_class = REST_PROXY_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (FlickrProxyPrivate));

  object_class->get_property = flickr_proxy_get_property;
  object_class->set_property = flickr_proxy_set_property;
  object_class->finalize = flickr_proxy_finalize;

  proxy_class->new_call = _new_call;

  pspec = g_param_spec_string ("consumer-key",  "consumer-key",
                               "The consumer key", NULL,
                               G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_CONSUMER_KEY,
                                   pspec);

  pspec = g_param_spec_string ("consumer-secret",  "consumer-secret",
                               "The consumer secret", NULL,
                               G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_CONSUMER_SECRET,
                                   pspec);

  pspec = g_param_spec_string ("token",  "token",
                               "The request or access token", NULL,
                               G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
                                   PROP_TOKEN,
                                   pspec);
}

static void
flickr_proxy_init (FlickrProxy *self)
{
  self->priv = PROXY_GET_PRIVATE (self);
}

RestProxy *
flickr_proxy_new (const char *consumer_key,
                 const char *consumer_secret)
{
  return flickr_proxy_new_with_token (consumer_key,
                                      consumer_secret,
                                      NULL);
}

RestProxy *
flickr_proxy_new_with_token (const char *consumer_key,
                             const char *consumer_secret,
                             const char *token)
{
  return g_object_new (FLICKR_TYPE_PROXY, 
                       "consumer-key", consumer_key,
                       "consumer-secret", consumer_secret,
                       "token", token,
                       "url-format", "http://api.flickr.com/services/rest/",
                       "binding-required", FALSE,
                       NULL);
}

/**
 * flickr_proxy_get_token:
 * @proxy: an #FlickrProxy
 *
 * Get the current request or access token.
 *
 * Returns: the token, or %NULL if there is no token yet.  This string is owned
 * by #FlickrProxy and should not be freed.
 */
const char *
flickr_proxy_get_token (FlickrProxy *proxy)
{
  FlickrProxyPrivate *priv = PROXY_GET_PRIVATE (proxy);
  return priv->token;
}

/**
 * flickr_proxy_set_token:
 * @proxy: an #FlickrProxy
 * @token: the access token
 *
 * Set the access token.
 */
void
flickr_proxy_set_token (FlickrProxy *proxy, const char *token)
{
  FlickrProxyPrivate *priv;

  g_return_if_fail (FLICKR_IS_PROXY (proxy));
  priv = PROXY_GET_PRIVATE (proxy);

  if (priv->token)
    g_free (priv->token);

  priv->token = g_strdup (token);
}

char *
flickr_proxy_sign (FlickrProxy *proxy, GHashTable *params)
{
  FlickrProxyPrivate *priv;
  GString *s;
  GList *keys;
  char *md5;

  g_return_val_if_fail (FLICKR_IS_PROXY (proxy), NULL);
  g_return_val_if_fail (params, NULL);

  priv = PROXY_GET_PRIVATE (proxy);

  s = g_string_new (priv->consumer_secret);

  keys = g_hash_table_get_keys (params);
  keys = g_list_sort (keys, (GCompareFunc)strcmp);

  while (keys) {
    const char *key;
    const char *value;

    key = keys->data;
    value = g_hash_table_lookup (params, key);

    g_string_append_printf (s, "%s%s", key, value);

    keys = keys->next;
  }

  md5 = g_compute_checksum_for_string (G_CHECKSUM_MD5, s->str, s->len);

  g_string_free (s, TRUE);

  return md5;
}

char *
flickr_proxy_build_login_url (FlickrProxy *proxy, const char *frob)
{
  SoupURI *uri;
  GHashTable *params;
  char *sig, *s;

  g_return_val_if_fail (FLICKR_IS_PROXY (proxy), NULL);
  g_return_val_if_fail (frob, NULL);

  uri = soup_uri_new ("http://flickr.com/services/auth/");
  params = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (params, "api_key", proxy->priv->consumer_key);
  /* TODO: parameter */
  g_hash_table_insert (params, "perms", "read");
  g_hash_table_insert (params, "frob", (gpointer)frob);

  sig = flickr_proxy_sign (proxy, params);
  g_hash_table_insert (params, "api_sig", sig);

  soup_uri_set_query_from_form (uri, params);

  s = soup_uri_to_string (uri, FALSE);

  g_free (sig);
  g_hash_table_unref (params);
  soup_uri_free (uri);

  return s;
}