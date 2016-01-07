#ifndef TOOLS_MERLINCAT_CONN_INFO_H_
#define TOOLS_MERLINCAT_CONN_INFO_H_

#include <glib.h>

/**
 * Information about a connection.
 */
struct ConnectionStorage_;
typedef struct ConnectionStorage_ ConnectionStorage;

ConnectionStorage *connection_new(
		void (*send)(gpointer, gconstpointer, gsize),
		void (*destroy)(gpointer),
		gboolean (*is_connected)(gpointer),
		gpointer user_data);
void connection_destroy(ConnectionStorage *cs);
void connection_send(ConnectionStorage *cs, gconstpointer data, gsize len);
gboolean connection_is_connected(ConnectionStorage *cs);

#endif /* TOOLS_MERLINCAT_CONN_INFO_H_ */
