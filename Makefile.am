AM_CFLAGS = -I$(top_builddir)

AM_CFLAGS += \
    $(glib_CFLAGS) \
    $(gio_CFLAGS) \
    $(gio_unix_CFLAGS) \
    -Wall -Werror

AM_LDFLAGS = \
    $(glib_LIBS) \
    $(gio_LIBS) \
    $(gio_unix_LIBS)

bin_PROGRAMS = can_proxy

can_proxy_SOURCES = \
    src/can.c src/can.h \
    src/client_gsource.c src/client_gsource.h \
    src/conn_info.c src/conn_info.h \
    src/main.c \
    src/server_gsource.c src/server_gsource.h
