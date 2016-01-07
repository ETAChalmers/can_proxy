/*
 * can.h
 *
 *  Created on: Oct 4, 2015
 *      Author: msikstrom
 */

#ifndef SRC_CAN_H_
#define SRC_CAN_H_

#include <glib.h>
#include <linux/can.h>

struct CanSource_;
typedef struct CanSource_ CanSource;

typedef void (*CanSourceCallback)(CanSource *, struct can_frame *, gpointer);

CanSource *cansource_new(const char *ifname, CanSourceCallback packet_callback,
		gpointer user_data);

void cansource_send(CanSource *cs, struct can_frame *frame);

#endif /* SRC_CAN_H_ */
