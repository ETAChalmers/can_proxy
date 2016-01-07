/*Some minor change*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <glib.h>
#include <glib-unix.h>
#include <gio/gio.h>
#include <glib/gprintf.h>

#include "can.h"
#include "client_gsource.h"
#include "server_gsource.h"

static gboolean stop_mainloop(gpointer data);

typedef struct ProxyConnStore_ {
    CanSource *can_src;
} ProxyConnStore;

/*
 * Command line options
 */
static gchar *opt_can_if = "can0";

static gboolean opt_listen = FALSE;
static gchar *opt_addr = "0.0.0.0";
static gint opt_port = 1200;

static GOptionEntry opt_entries[] = {
        { "canif", 'i', 0, G_OPTION_ARG_STRING, &opt_can_if, "bind to can interface", "interface"},
        { "listen", 'l', 0, G_OPTION_ARG_NONE, &opt_listen, "listen for tcp connections", NULL},
		{ "addr", 'a', 0, G_OPTION_ARG_STRING, &opt_addr, "connect to address / bind to address", "addr" },
		{ "port", 'p', 0, G_OPTION_ARG_INT, &opt_port, "connecto to port / listen to port", "port" },
		{ NULL }
};

static gpointer net_conn_new(ConnectionStorage *conn, gpointer user_data);
static void net_conn_data(ConnectionStorage *conn, gpointer buffer, gsize length, gpointer conn_user_data);
static void net_conn_close(gpointer conn_user_data);

static void can_new_frame(CanSource *can_src, struct can_frame *frame, gpointer user_data);

int main(int argc, char *argv[]) {
	GMainLoop *ml = NULL;
	GError *error = NULL;

    ProxyConnStore stor;

    ClientSource *cli_src = NULL;
    ServerSource *srv_src = NULL;

	GOptionContext *optctx;

    memset(&stor, 0, sizeof(ProxyConnStore));

	optctx = g_option_context_new("- CanSocket / Auml-CAN proxy");
	g_option_context_add_main_entries(optctx, opt_entries, NULL);
	if (!g_option_context_parse(optctx, &argc, &argv, &error)) {
		g_print("option parsing failed: %s\n", error->message);
		exit(1);
	}

	ml = g_main_loop_new(NULL, TRUE);

	/* Do a nice shutdown if interrupted */
	g_unix_signal_add(SIGHUP, stop_mainloop, ml);
	g_unix_signal_add(SIGINT, stop_mainloop, ml);
	g_unix_signal_add(SIGTERM, stop_mainloop, ml);

	stor.can_src = cansource_new(opt_can_if, can_new_frame, &stor);

    if(opt_listen) {
        srv_src = server_source_new(opt_addr, opt_port, net_conn_new, net_conn_data, net_conn_close, &stor);
        if (srv_src == NULL) {
            g_warning("Can't bind to port");
            goto cleanup;
        }
    } else {
        cli_src = client_source_new(opt_addr, opt_port, net_conn_new, net_conn_data, net_conn_close, &stor);
        if (cli_src == NULL) {
            g_warning("Can't connect to client");
            goto cleanup;
        }
    }

	g_main_loop_run(ml);

cleanup:
    server_source_destroy(srv_src);
    client_source_destroy(cli_src);

	g_option_context_free(optctx);
	g_main_loop_unref(ml);

	return 0;
}

static gboolean stop_mainloop(gpointer data) {
	GMainLoop *ml = (GMainLoop*) data;
	g_main_loop_quit(ml);
	return G_SOURCE_CONTINUE;
}

static gpointer net_conn_new(ConnectionStorage *conn, gpointer user_data) {
    g_message("Got connection");
    return user_data;
}
static void net_conn_data(ConnectionStorage *conn, gpointer buffer, gsize length, gpointer conn_user_data) {
    g_message("Got data");
}
static void net_conn_close(gpointer conn_user_data) {
    g_message("Lost connection");
}

static void can_new_frame(CanSource *can_src, struct can_frame *frame, gpointer user_data) {
}
