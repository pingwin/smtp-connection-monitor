bin_PROGRAMS = stub-monitor

stub_monitor_SOURCES = main.c

stub_monitor_LDFLAGS = @LDFLAGS@

INCLUDE = -I($top_srcdir)/src -I($top_srcdir)/src/includes $(all_includes)


SUBDIRS = includes

# add debugging
AM_CFLAGS = -ggdb -g3 
#-Wall
