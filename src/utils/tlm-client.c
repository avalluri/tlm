/* vi: set et sw=4 ts=4 cino=t0,(0: */
/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of tlm (Tizen Login Manager)
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * Contact: Imran Zaman <imran.zaman@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include "config.h"
#include <check.h>
#include <error.h>
#include <errno.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <glib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib-unix.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"

#include "common/dbus/tlm-dbus.h"
#include "common/tlm-log.h"
#include "common/tlm-config.h"
#include "common/dbus/tlm-dbus-login-gen.h"
#include "common/tlm-utils.h"
#include "common/dbus/tlm-dbus-utils.h"

static GPid daemon_pid = 0;

//static GMainLoop *main_loop = NULL;

typedef struct {
    gchar *username;
    gchar *password;
    gchar **environment;
} TlmUser;

static TlmUser *
_create_tlm_user ()
{
    return g_malloc0 (sizeof (TlmUser));
}

static void
_free_tlm_user (
        TlmUser *user)
{
    if (user) {
        g_free (user->username); g_free (user->password);
        g_strfreev (user->environment);
        g_free (user);
    }
}

#if 0
static void
_stop_mainloop ()
{
    if (main_loop) {
        g_main_loop_quit (main_loop);
    }
}

static void
_create_mainloop ()
{
    if (main_loop == NULL) {
        main_loop = g_main_loop_new (NULL, FALSE);
    }
}

static void
_run_mainloop ()
{
    if (main_loop) {
        g_main_loop_run (main_loop);
    }
}
#endif

static gboolean
_setup_daemon ()
{
    DBG ("");

    GError *error = NULL;
    gchar *argv[2];

    const gchar *bin_path = TLM_BIN_DIR;

#ifdef ENABLE_DEBUG
    const gchar *env_val = g_getenv("TLM_BIN_DIR");
    if (env_val)
        bin_path = env_val;
#endif
    if(!bin_path) {
        WARN("No TLM daemon bin path found");
        return FALSE;
    }

    gchar *test_daemon_path = g_build_filename (bin_path, "tlm", NULL);
    if(!test_daemon_path) {
        WARN("No TLM daemon path found");
        return FALSE;
    }

    argv[0] = test_daemon_path;
    argv[1] = NULL;
    g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
            &daemon_pid, &error);
    g_free (test_daemon_path);
    if (error) {
        WARN("Failed to spawn daemon : %d:%s", error->code, error->message);
        g_error_free (error);
        error = NULL;
        return FALSE;
    }
    sleep (5); /* 5 seconds */

    DBG ("Daemon PID = %d\n", daemon_pid);
    return TRUE;
}

static void
_teardown_daemon ()
{
    if (daemon_pid) kill (daemon_pid, SIGTERM);
}

GDBusConnection *
_get_root_socket_bus_connection (
        GError **error)
{
    gchar address[128];
    g_snprintf (address, 127, TLM_DBUS_ROOT_SOCKET_ADDRESS);
    return g_dbus_connection_new_for_address_sync (address,
            G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT, NULL, NULL, error);
}

GDBusConnection *
_get_bus_connection (
        const gchar *seat_id,
        GError **error)
{
    uid_t user_id = getuid ();

    if (user_id == 0) {
        return _get_root_socket_bus_connection (error);
    }

    /* get dbus connection for specific user only */
    gchar address[128];
    g_snprintf (address, 127, "unix:path=%s/%s-%d", TLM_DBUS_SOCKET_PATH,
            seat_id, user_id);
    return g_dbus_connection_new_for_address_sync (address,
            G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT, NULL, NULL, error);
}

TlmDbusLogin *
_get_login_object (
        GDBusConnection *connection,
        GError **error)
{
    return tlm_dbus_login_proxy_new_sync (connection, G_DBUS_PROXY_FLAGS_NONE,
            NULL, TLM_LOGIN_OBJECTPATH, NULL, error);
}

static GVariant *
_get_session_property (
        GDBusProxy *proxy,
        const gchar *object_path,
        const gchar *prop_key)
{
    GError *error = NULL;
    GVariant *result = NULL;
    GVariant *prop_value = NULL;

    result = g_dbus_connection_call_sync (
            g_dbus_proxy_get_connection (proxy),
            g_dbus_proxy_get_name (proxy),
            object_path,
            "org.freedesktop.DBus.Properties",
            "GetAll",
            g_variant_new ("(s)",  "org.freedesktop.login1.Session"),
            G_VARIANT_TYPE ("(a{sv})"),
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error);
    if (error) {
        DBG ("Failed with error %d:%s", error->code, error->message);
        g_error_free (error);
        error = NULL;
        return NULL;
    }

    if (g_variant_is_of_type (result, G_VARIANT_TYPE ("(a{sv})"))) {
        GVariantIter *iter = NULL;
        GVariant *item = NULL;
        g_variant_get (result, "(a{sv})",  &iter);
        while ((item = g_variant_iter_next_value (iter)))  {
            gchar *key;
            GVariant *value;
            g_variant_get (item, "{sv}", &key, &value);
            if (g_strcmp0 (key, prop_key) == 0) {
                prop_value = value;
                g_free (key); key = NULL;
                break;
            }
            g_free (key); key = NULL;
            g_variant_unref (value); value = NULL;
        }
    }
    g_variant_unref (result);
    return prop_value;
}

static GVariant *
_get_property (
        const gchar *prop_key)
{
    GError *error = NULL;
    GDBusProxy *proxy = NULL;
    GVariant *res = NULL;
    GVariant *prop_value = NULL;

    GDBusConnection *connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL,
            &error);
    if (error) goto _finished;

    proxy = g_dbus_proxy_new_sync (connection,
            G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES, NULL,
            "org.freedesktop.login1", //destination
            "/org/freedesktop/login1", //path
            "org.freedesktop.login1.Manager", //interface
            NULL, &error);
    if (error) goto _finished;

    res = g_dbus_proxy_call_sync (proxy, "GetSessionByPID",
            g_variant_new ("(u)", getpid()), G_DBUS_CALL_FLAGS_NONE, -1,
            NULL, &error);
    if (res) {
        gchar *obj_path = NULL;
        g_variant_get (res, "(o)", &obj_path);
        prop_value = _get_session_property (proxy, obj_path, prop_key);
        g_variant_unref (res); res = NULL;
        g_free (obj_path);
    }

_finished:
    if (error) {
        DBG ("failed to listen for login events: %s", error->message);
        g_error_free (error);
    }
    if (proxy) g_object_unref (proxy);
    if (connection) g_object_unref (connection);

    return prop_value;
}

static GVariant *
_convert_environ_to_variant (gchar **env) {

    GVariantBuilder *builder = NULL;
    GVariant *venv = NULL;
    gchar **penv = env;

    g_return_val_if_fail (env != NULL, NULL);

    builder = g_variant_builder_new (((const GVariantType *) "a{ss}"));

    while (*penv) {
        gchar *key = *penv++;
        gchar *value = *penv++;
        if (!key || !value) {
            break;
        }
        g_variant_builder_add (builder, "{ss}", key, value);
    }
    venv = g_variant_builder_end (builder);
    g_variant_builder_unref (builder);

    return venv;
}

static void
_handle_user_login (
        TlmUser *user)
{
    DBG ("");
    GError *error = NULL;
    GDBusConnection *connection = NULL;
    TlmDbusLogin *login_object = NULL;
    GVariant *venv = NULL;
    GVariant *vseat = NULL;
    gchar *seat = NULL;

    if (!user || !user->username || !user->password) {
        WARN("Invalid username/password");
        return;
    }

    vseat = _get_property ("Seat");
    if (!vseat) {
        WARN("Unable to retrieve seat information");
        return;
    }
    g_variant_get (vseat, "(so)", &seat, NULL);
    g_variant_unref (vseat);

    connection = _get_bus_connection (seat, &error);
    if (connection == NULL) {
        WARN("failed to get bus connection : error %s",
            error ? error->message : "(null)");
        goto _finished;
    }

    login_object = _get_login_object (connection, &error);
    if (login_object == NULL) {
        WARN("failed to get login object : error %s",
            error ? error->message : "(null)");
        goto _finished;
    }

    venv = _convert_environ_to_variant (user->environment);
    if (venv == NULL) {
        WARN("failed to get user environemnt");
        goto _finished;
    }

    tlm_dbus_login_call_login_user_sync (login_object, seat, user->username,
            user->password, venv, NULL, &error);
    if (error) {
        WARN ("login failed with error: %d:%s", error->code, error->message);
        g_error_free (error);
        error = NULL;
    } else {
        DBG ("User logged in successfully");
    }

_finished:
    g_free (seat);
    if (login_object) g_object_unref (login_object);
    if (connection) g_object_unref (connection);
}

static void
_handle_user_logout (
        TlmUser *user)
{
    DBG ("\n");
    GError *error = NULL;
    GDBusConnection *connection = NULL;
    TlmDbusLogin *login_object = NULL;
    GVariant *vseat = NULL;
    gchar *seat = NULL;

    if (!user) {
        WARN("Invalid user");
        return;
    }

    vseat = _get_property ("Seat");
    if (!vseat) {
        WARN("Unable to retrieve seat information");
        return;
    }

    g_variant_get (vseat, "(so)", &seat, NULL);
    g_variant_unref (vseat);

    connection = _get_bus_connection (seat, &error);
    if (connection == NULL) {
        WARN("failed to get bus connection : error %s",
            error ? error->message : "(null)");
        goto _finished;
    }

    login_object = _get_login_object (connection, &error);
    if (login_object == NULL) {
        WARN("failed to get login object : error %s",
            error ? error->message : "(null)");
        goto _finished;
    }

    tlm_dbus_login_call_logout_user_sync (login_object, seat,  NULL, &error);
    if (error) {
        WARN ("logout failed with error: %d:%s", error->code, error->message);
        g_error_free (error);
        error = NULL;
    } else {
        DBG ("User logged out successfully");
    }

_finished:
    g_free (seat);
    if (login_object) g_object_unref (login_object);
    if (connection) g_object_unref (connection);
}

static void
_handle_user_switch (
        TlmUser *user)
{
    DBG ("\n");
    GError *error = NULL;
    GDBusConnection *connection = NULL;
    TlmDbusLogin *login_object = NULL;
    GVariant *venv = NULL;
    GVariant *vseat = NULL;
    gchar *seat = NULL;

    if (!user || !user->username || !user->password) {
        WARN("Invalid username/password");
        return;
    }

    vseat = _get_property ("Seat");
    if (!vseat) {
        WARN("Unable to retrieve seat information");
        return;
    }
    g_variant_get (vseat, "(so)", &seat, NULL);
    g_variant_unref (vseat);

    connection = _get_bus_connection (seat, &error);
    if (connection == NULL) {
        WARN("failed to get bus connection : error %s",
            error ? error->message : "(null)");
        goto _finished;
    }

    login_object = _get_login_object (connection, &error);
    if (login_object == NULL) {
        WARN("failed to get login object : error %s",
            error ? error->message : "(null)");
        goto _finished;
    }

    venv = _convert_environ_to_variant (user->environment);
    if (venv == NULL) {
        WARN("failed to get user environemnt");
        goto _finished;
    }

    tlm_dbus_login_call_switch_user_sync (login_object, seat, user->username,
            user->password, venv, NULL, &error);
    if (error) {
        WARN ("switch user failed with error: %d:%s", error->code,
                error->message);
        g_error_free (error);
        error = NULL;
    } else {
        DBG ("User switched in successfully");
    }

_finished:
    g_free (seat);
    if (login_object) g_object_unref (login_object);
    if (connection) g_object_unref (connection);
}

int main (int argc, char *argv[])
{
    GError *error = NULL;
    GOptionContext *context;
    gboolean rval = FALSE;

    gboolean is_user_login_op = FALSE, is_user_logout_op = FALSE;
    gboolean is_user_switch_op = FALSE;
    gboolean run_tlm_daemon = FALSE;
    GOptionGroup* user_option = NULL;
    TlmUser *user = _create_tlm_user ();

    GOptionEntry main_entries[] =
    {
        { "login-user", 'l', 0, G_OPTION_ARG_NONE, &is_user_login_op,
                "login user -- username and password is mandatory", NULL },
        { "logout-user", 'o', 0, G_OPTION_ARG_NONE, &is_user_logout_op,
                "logout user -- seatid will be determined by GetSessionByPID",
                NULL },
        { "switch-user", 's', 0, G_OPTION_ARG_NONE, &is_user_switch_op,
                "switch user -- username and password is mandatory", NULL },
        { "run-daemon", 'r', 0, G_OPTION_ARG_NONE, &run_tlm_daemon,
                "run tlm daemon (by default tlm daemon is not run)",
                NULL },
        { NULL }
    };

    GOptionEntry user_entries[] =
    {
        { "username", 0, 0, G_OPTION_ARG_STRING, &user->username,
                "user name", "user1" },
        { "password", 0, 0, G_OPTION_ARG_STRING, &user->password,
                "user password", "mypass" },
        { "env", 0, 0, G_OPTION_ARG_STRING_ARRAY, &user->environment,
                "user environment", "a 1 b 2" },
        { NULL }
    };

#if !GLIB_CHECK_VERSION (2, 36, 0)
    g_type_init ();
#endif

    context = g_option_context_new (" [tlm client Option]\n"
            "  e.g. To login a user, ./tlm-client -l --username user1 "
            "--password p1 --env key1 val1 key2 val2");
    g_option_context_add_main_entries (context, main_entries, NULL);

    user_option = g_option_group_new ("user-options", "User specific options",
            "User specific options", NULL, NULL);
    g_option_group_add_entries (user_option, user_entries);
    g_option_context_add_group (context, user_option);

    rval = g_option_context_parse (context, &argc, &argv, &error);
    g_option_context_free (context);
    if (!rval) {
        DBG ("option parsing failed: %s\n", error->message);
        _free_tlm_user (user);
        return EXIT_FAILURE;
    }

    if (geteuid() != 0) {
        WARN("test-client can only be run with ROOT privileges");
        _free_tlm_user (user);
        return EXIT_FAILURE;
    }

    if (run_tlm_daemon)
        _setup_daemon ();

    if (is_user_login_op) {
        _handle_user_login (user);
    } else if (is_user_logout_op) {
        _handle_user_logout (user);
    } else if (is_user_switch_op) {
        _handle_user_switch (user);
    } else {
        WARN ("No option specified");
    }
    _free_tlm_user (user);

    if (run_tlm_daemon)
        _teardown_daemon ();
    return EXIT_SUCCESS;
}