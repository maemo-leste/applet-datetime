controlpanelpluginlib_LTLIBRARIES = libcpdatetime.la

libcpdatetime_la_CFLAGS = $(OSSO_CFLAGS) $(HILDON_CFLAGS) $(CPL_CFLAGS) \
			  $(CITYINFO_CFLAGS) $(TIME_CFLAGS) \
			  $(CLOCKCORE_CFLAGS) $(HILDON_TZ_CFLAGS) \
			  -DG_LOG_DOMAIN=\"$(PACKAGE)\" \
			  -DOSSOLOG_COMPILE=1 -DLOCALE_DIR=\"$(localedir)\"

libcpdatetime_la_LDFLAGS = -Wl,--as-needed $(OSSO_LIBS) $(HILDON_LIBS) \
			   $(CITYINFO_LIBS) $(CPL_LIBS) $(TIME_LIBS) \
			   $(CLOCKCORE_LIBS) $(HILDON_TZ_LIBS)\
			   -module -avoid-version

libcpdatetime_la_SOURCES = cpdatetime.c

controlpanelplugindesktopentry_DATA = cpdatetime.desktop
EXTRA_DIST = $(controlpanelplugindesktopentry_DATA)

MAINTAINERCLEANFILES = Makefile.in
