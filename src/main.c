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

/* Maximum line length, longer lines will be truncated */
#define LINEBUF_SIZE 256

static gboolean stop_mainloop(gpointer data);

typedef struct ProxyConnStore_ {
    CanSource *can_src;
    GPtrArray *connections;
} ProxyConnStore;

typedef struct ProxyConn_ {
    ProxyConnStore *pcs;
    ConnectionStorage *conn;

    gint bufpos;
    gchar buf[LINEBUF_SIZE];
} ProxyConn;

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

static void net_new_line(ProxyConn *pc, const gchar *line);
static void net_send_frame(ProxyConn *pc, struct can_frame *frame);

static void can_new_frame(CanSource *can_src, struct can_frame *frame, gpointer user_data);

int main(int argc, char *argv[]) {
	GMainLoop *ml = NULL;
	GError *error = NULL;

    ProxyConnStore stor;

    ClientSource *cli_src = NULL;
    ServerSource *srv_src = NULL;

	GOptionContext *optctx;

    memset(&stor, 0, sizeof(ProxyConnStore));
    stor.connections = g_ptr_array_new(); /* FIXME: should disconnect remaining connecitons */

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

    g_ptr_array_unref(stor.connections);

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
    ProxyConn *pc = g_malloc0(sizeof(ProxyConn));

    pc->pcs = (ProxyConnStore*) user_data;
    pc->conn = conn;
    pc->bufpos = 0;

    g_ptr_array_add(pc->pcs->connections, pc);
    return pc;
}
static void net_conn_data(ConnectionStorage *conn, gpointer buffer, gsize length, gpointer conn_user_data) {
    ProxyConn *pc = (ProxyConn *)conn_user_data;
    gchar *strbuf = buffer;
    gint i;
    for(i=0;i<length;i++) {
        if(strbuf[i] == '\n') {
            /* New line */
            pc->buf[pc->bufpos] = '\0';
            pc->bufpos = 0;

            net_new_line(pc, pc->buf);
        } else {
            /* always keep space for \0 */
            if(pc->bufpos < LINEBUF_SIZE-1) {
                pc->buf[pc->bufpos++] = strbuf[i];
            }
        }
    }
}
static void net_conn_close(gpointer conn_user_data) {
    ProxyConn *pc = (ProxyConn *)conn_user_data;

    g_ptr_array_remove_fast(pc->pcs->connections, pc);

    g_free(pc);
}

static void net_new_line(ProxyConn *pc, const gchar *line) {
    gchar **parts = g_strsplit_set(line, " ", 0);
    struct can_frame frame;
    gint i;

    memset(&frame, 0, sizeof(struct can_frame));

    /* If no PKT line, ignore */
    if(parts[0] == NULL || strcmp("PKT", parts[0]) != 0)
        goto cleanup;

    /* Parse ID */
    if(parts[1] == NULL)
        goto cleanup;
    frame.can_id = g_ascii_strtoll(parts[1], NULL, 16) & CAN_EFF_MASK;

    /* Parse extended flag */
    if(parts[2] == NULL || (strcmp(parts[2], "0") != 0 && strcmp(parts[2], "1") != 0))
        goto cleanup;
    if(parts[2][0] == '1')
        frame.can_id |= CAN_EFF_FLAG;
    else
        frame.can_id &= CAN_SFF_MASK; /* truncate id if no eff */

    /* Parse remote request flag */
    if(parts[3] == NULL || (strcmp(parts[3], "0") != 0 && strcmp(parts[3], "1") != 0))
        goto cleanup;
    if(parts[3][0] == '1')
        frame.can_id |= CAN_RTR_FLAG;

    /* Parse data fields */
    for(i=0;parts[4+i] != NULL && i<8;i++) {
        frame.data[i] = g_ascii_strtoll(parts[4+i], NULL, 16);
    }
    frame.can_dlc = i;

    /* Successful parse, send frame */
    for(i=0;i<pc->pcs->connections->len;i++) {
        /* Don't echo back */
        if(pc->pcs->connections->pdata[i] == pc)
            continue;
        net_send_frame(pc->pcs->connections->pdata[i], &frame);
    }

    /* also send to SocketCan */
    cansource_send(pc->pcs->can_src, &frame);

cleanup:
    g_strfreev(parts);
}

static void net_send_frame(ProxyConn *pc, struct can_frame *frame) {
    gchar buf[256];
    gsize pos=0;
    gint i;
    gchar tohex[16] = {
        '0','1','2','3',
        '4','5','6','7',
        '8','9','a','b',
        'c','d','e','f'
    };

    buf[pos++] = 'P';
    buf[pos++] = 'K';
    buf[pos++] = 'T';
    buf[pos++] = ' ';
    buf[pos++] = tohex[(frame->can_id>>(4*7))&0x1];
    buf[pos++] = tohex[(frame->can_id>>(4*6))&0xF];
    buf[pos++] = tohex[(frame->can_id>>(4*5))&0xF];
    buf[pos++] = tohex[(frame->can_id>>(4*4))&0xF];
    buf[pos++] = tohex[(frame->can_id>>(4*3))&0xF];
    buf[pos++] = tohex[(frame->can_id>>(4*2))&0xF];
    buf[pos++] = tohex[(frame->can_id>>(4*1))&0xF];
    buf[pos++] = tohex[(frame->can_id>>(4*0))&0xF];
    buf[pos++] = ' ';
    buf[pos++] = (frame->can_id&CAN_EFF_FLAG) ? '1' : '0';
    buf[pos++] = ' ';
    buf[pos++] = (frame->can_id&CAN_RTR_FLAG) ? '1' : '0';
    for(i=0;i<frame->can_dlc;i++) {
        buf[pos++] = ' ';
        buf[pos++] = tohex[(frame->data[i]>>4)&0xF];
        buf[pos++] = tohex[(frame->data[i]>>0)&0xF];
    }
    buf[pos++] = '\n';
    connection_send(pc->conn, buf, pos);
}

static void can_new_frame(CanSource *can_src, struct can_frame *frame, gpointer user_data) {
    ProxyConnStore *pcs = (ProxyConnStore *)user_data;
    gint i;
    for(i=0;i<pcs->connections->len;i++) {
        net_send_frame(pcs->connections->pdata[i], frame);
    }
}
