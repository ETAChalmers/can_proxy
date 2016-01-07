#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([can_proxy], [v0.0.1-beta1], [])
AM_INIT_AUTOMAKE([subdir-objects])

AM_SILENT_RULES([yes])

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.
PKG_CHECK_MODULES([check], [check])

PKG_CHECK_MODULES([glib], [glib-2.0])
PKG_CHECK_MODULES([gio], [gio-2.0])
PKG_CHECK_MODULES([gio_unix], [gio-unix-2.0])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([
	Makefile
	])

AC_OUTPUT
