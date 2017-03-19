AM_CFLAGS = $(MORE_CFLAGS)
AM_CPPFLAGS = $(MORE_CPPFLAGS)
AM_LDFLAGS = $(MORE_LDFLAGS)

bin_PROGRAMS = $(top_builddir)/bin/crow

__top_builddir__bin_crow_SOURCES  = rest.h rest.c commander.c commander.h log.c log.h crow.h crow.c
__top_builddir__bin_crow_CFLAGS   = $(AM_CFLAGS) @libcurl_CFLAGS@ @libjson_CFLAGS@
__top_builddir__bin_crow_LDFLAGS  = $(AM_LDFLAGS) @libcurl_LIBS@ @libjson_LIBS@ -lwsclient