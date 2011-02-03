/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

//gcc -o raw-dbus raw-dbus.c `pkg-config --cflags --libs dbus-1 glib-2.0` -Wall -Wuninitialized -lm -Werror -g -fexceptions && ./raw-dbus

#include <glib.h>
#include <dbus/dbus.h>

/**
 * helper_dict_add_property:
 **/
static void
helper_dict_add_property (DBusMessageIter *dict,
			  const gchar *key,
			  const gchar *value)
{

	DBusMessageIter entry;
	dbus_message_iter_open_container(dict,
					 DBUS_TYPE_DICT_ENTRY,
					 NULL,
					 &entry);
	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
	dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &value);
	dbus_message_iter_close_container(dict, &entry);
}

/**
 * main:
 **/
int
main (int argc, char **argv)
{
	const gchar *device_id;
	const gchar *device_path_tmp;
	DBusConnection *con;
	DBusError error;
	DBusMessageIter args;
	DBusMessageIter dict;
	DBusMessage *message = NULL;
	DBusMessage *reply = NULL;
	GMainLoop *loop;
	guint options = 1;

	/* connect to system bus */
	con = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (con == NULL) {
		g_warning("failed to connect to system bus");
		goto out;
	}

	/* this is unique to the device */
	device_id = "hello-dave";
	message = dbus_message_new_method_call("org.freedesktop.ColorManager",
					       "/org/freedesktop/ColorManager",
					       "org.freedesktop.ColorManager",
					       "CreateDevice");
	dbus_message_iter_init_append(message, &args);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &device_id);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &options);

	/* set initial properties */
	dbus_message_iter_open_container(&args,
					 DBUS_TYPE_ARRAY,
					 "{ss}",
					 &dict);
	helper_dict_add_property(&dict, "Colorspace", "RGB");
	helper_dict_add_property(&dict, "Kind", "scanner");
	dbus_message_iter_close_container(&args, &dict);

	/* send syncronous */
	dbus_error_init(&error);
	g_debug("Calling CreateDevice(%s,%d)",
		device_id, options);
	reply = dbus_connection_send_with_reply_and_block(con,
							  message,
							  -1,
							  &error);
	if (reply == NULL) {
		g_warning("failed to send: %s:%s",
			  error.name, error.message);
		dbus_error_free(&error);
		goto out;
	}

	/* get reply data */
        dbus_message_iter_init(reply, &args);
	if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_OBJECT_PATH) {
		g_warning("incorrect reply type");
		goto out;
	}
        dbus_message_iter_get_basic(&args, &device_path_tmp);
	g_debug("created device %s", device_path_tmp);

	/* just spin in a main loop */
	loop = g_main_loop_new(NULL, TRUE);
	g_main_loop_unref(loop);
out:
	if (con != NULL)
		dbus_connection_unref(con);
	if (message != NULL)
		dbus_message_unref(message);
	if (reply != NULL)
		dbus_message_unref(reply);
	return 0;
}
