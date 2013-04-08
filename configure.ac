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

AC_CHECK_HEADER(ssl.h, [], [AC_MSG_ERROR(You need the ssl headers)])
AC_CHECK_LIB([ssl], [SSL_library_init], [], [AC_MSG_ERROR(Required libssl missing)])
AC_CHECK_LIB([crypto], [BN_bn2bin], [], [AC_MSG_ERROR(Required libcrypto missing)])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT