## Process this file with automake to produce Makefile.in.

INCLUDES = @INCLUDES@ -I.. -I$(top_srcdir) -I$(top_srcdir)/lib @SNMP_INCLUDES@
DEFS = @DEFS@ $(LOCAL_OPTS) -DSYSCONFDIR=\"$(sysconfdir)/\"
INSTALL_SDATA=@INSTALL@ -m 600

lib_LTLIBRARIES = libospf.la
libospf_la_LDFLAGS = -version 0:0:0

sbin_PROGRAMS = ospfd

libospf_la_SOURCES = \
	zapd_zebra.c zapd_vty.c zapd.c

ospfdheaderdir = $(pkgincludedir)/ospfd

ospfdheader_HEADERS = \
	zapd_zebra.h  zapd_vty.h zapd.h

noinst_HEADERS =

ospfd_SOURCES = zapd_main.c

ospfd_LDADD = libospf.la ../lib/libzebra.la @LIBCAP@

EXTRA_DIST = OSPF-MIB.txt OSPF-TRAP-MIB.txt ChangeLog.opaque.txt

examplesdir = $(exampledir)
dist_examples_DATA = zapd.conf.sample

