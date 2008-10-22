AC_INIT(src/main.c);
AM_INIT_AUTOMAKE(stub-monitor__c,0.3)
AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_PROG_INSTALL
#AC_PROG_LIBTOOL

#AC_CHECK_LIB(pthread, pthread_create, [], [
#	echo "Error! You need pthread support."
#	exit -1
#	])

test `uname -s` = "Linux" && AC_DEFINE(GNU_LINUX, 1, [Is a GNU-Linux system])
test `uname -s` = "FreeBSD" && AC_DEFINE(FREEBSD, 1, [Is a FreeBSD system])
test `uname -s` = "Darwin" && AC_DEFINE(DARWIN, 1, [Is a Darwin system]) AC_DEFINE(FREEBSD, 1, [Darwin is FreeBSD pretty Much])


AC_CHECK_LIB(event, event_add, [], [
	echo "Error! You need libevent support."
	exit -1
	])

AC_DEFINE(EXIT_FAILURE, 1,[something])
AC_DEFINE(EXIT_SUCCESS, 0, [something else])
AC_DEFINE(APP_VERSION, "0.1", [Application Version])

AC_OUTPUT(Makefile src/Makefile src/includes/Makefile)
