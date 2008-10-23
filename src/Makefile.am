bin_PROGRAMS = stub-monitor stub-watcher

stub_monitor_SOURCES = main.c \
	includes/XKSignals.h includes/XKSignals.c

stub_watcher_SOURCES = client.c
stub_monitor_LDFLAGS = @LDFLAGS@

INCLUDE = -I($top_srcdir)/src -I($top_srcdir)/src/includes $(all_includes)


SUBDIRS = includes

# add debugging
#AM_CFLAGS = -ggdb -g3
#-Wall
