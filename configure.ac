AC_INIT([opencapwap], [1.0], [info@travelping.com])
AM_INIT_AUTOMAKE([1.9 foreign silent-rules])
AC_CONFIG_MACRO_DIR([m4])
AC_USE_SYSTEM_EXTENSIONS

LT_INIT

AM_CONFIG_HEADER(config.h)
AC_PREFIX_DEFAULT(/usr)

AC_CANONICAL_HOST
AC_USE_SYSTEM_EXTENSIONS

AM_SILENT_RULES([yes])

AC_PROG_LIBTOOL

AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O

AC_PROG_INSTALL
AC_SET_MAKE

AC_CHECK_HEADER([pthread.h], [], [AC_MSG_ERROR(You need the pthread headers)])
AC_CHECK_LIB([pthread], [pthread_create], [], [AC_MSG_ERROR(You need the pthread library)])

AC_CHECK_FUNC([sem_timedwait], [AC_DEFINE([HAVE_SEM_TIMEDWAIT], [1],
                               [Define if sem_timedwait exists.])])

AC_ARG_ENABLE([debug],
    AS_HELP_STRING([--enable-debug], [enable debug]),
    [CFLAGS="$CFLAGS -DCW_DEBUGGING"], [])

AC_ARG_ENABLE([dtls],
    AS_HELP_STRING([--disable-dtls], [disable DTLS support]))

AM_CONDITIONAL([USE_SSL], [test "x$enable_dtls" != "xno"])
AS_IF([test "x$enable_dtls" != "xno"], [
  AX_CHECK_OPENSSL([], [AC_MSG_ERROR(OpenSSL not found!)])
], [
  CFLAGS="$CFLAGS -DCW_NO_DTLS -DCW_NO_DTLSCWParseConfigurationUpdateRequest"
])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT