/*
 * can.c
 *
 *  Created on: Oct 4, 2015
 *      Author: msikstrom
 */

#include "can.h"

#include <string.h>
#include <stdio.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <glib.h>
#include <gio/gio.h>

struct CanSource_ {
	GSource parent;
	int fd;

	int got_frame;
	struct can_frame frame;
};

static gboolean can_source_prepare(GSource *src, gint *timeout);
static gboolean can_source_check(GSource *src);
static gboolean can_source_dispatch(GSource *src, GSourceFunc callback,
		gpointer user_data);
static void can_source_finalize(GSource *src);

static GSourceFuncs can_source_funcs = { .prepare = can_source_prepare, .check =
		can_source_check, .dispatch = can_source_dispatch, .finalize =
		can_source_finalize };

CanSource *cansource_new(const char *ifname, CanSourceCallback packet_callback,
		gpointer user_data) {
	CanSource *cs;
	struct ifreq ifr;
	struct sockaddr_can addr;
	int flags;

	cs = (CanSource*) g_source_new(&can_source_funcs, sizeof(CanSource));

	cs->fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (cs->fd < 0)
		goto err;

	strcpy(ifr.ifr_name, ifname);
	ioctl(cs->fd, SIOCGIFINDEX, &ifr);

	flags = fcntl(cs->fd, F_GETFL, 0);
	fcntl(cs->fd, F_SETFL, flags | O_NONBLOCK);

	memset(&addr, 0, sizeof(struct sockaddr_can));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(cs->fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		goto err;
	}

	g_source_add_unix_fd((GSource *) cs, cs->fd, G_IO_IN | G_IO_ERR);
	g_source_set_callback((GSource *) cs, (GSourceFunc) packet_callback,
			user_data, NULL);

	g_source_attach((GSource *) cs, NULL);

	return cs;
	err: g_source_unref((GSource *) cs);
	return NULL;
}

static gboolean can_source_prepare(GSource *src, gint *timeout) {
	return FALSE;
}
static gboolean can_source_check(GSource *src) {
	CanSource *cs = (CanSource *) src;
	ssize_t nbytes_read;

	nbytes_read = read(cs->fd, &cs->frame, sizeof(struct can_frame));

	if (nbytes_read > 0) {
		return TRUE;
	}

	return FALSE;
}
static gboolean can_source_dispatch(GSource *src, GSourceFunc callback,
		gpointer user_data) {
	CanSource *cs = (CanSource *) src;
	CanSourceCallback packet_callback = (CanSourceCallback) callback;

	(*packet_callback)(cs, &cs->frame, user_data);
	return TRUE;
}
static void can_source_finalize(GSource *src) {
}

void cansource_send(CanSource *cs, struct can_frame *frame) {
	write(cs->fd, frame, sizeof(struct can_frame));
}
