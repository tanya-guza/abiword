
noinst_LTLIBRARIES = libunix.la

AM_CPPFLAGS = \
	$(COLLAB_CFLAGS) \
	-I$(srcdir)/.. \
	-I$(srcdir)/../.. \
	-I$(srcdir)/../../.. \
	-I$(srcdir)/../../../core

libunix_la_SOURCES = \
	ap_UnixDialog_GenericInput.cpp \
	ap_UnixDialog_GenericInput.h \
	ap_UnixDialog_GenericProgress.cpp \
	ap_UnixDialog_GenericProgress.h \
	ServiceUnixAccountHandler.cpp \
	ServiceUnixAccountHandler.h

uidir=$(ABIWORD_UIDIR)
if TOOLKIT_GTK2
ui_DATA = \
	ap_UnixDialog_GenericInput-2.ui \
	ap_UnixDialog_GenericProgress-2.ui
else
ui_DATA = \
	ap_UnixDialog_GenericInput.ui \
	ap_UnixDialog_GenericProgress.ui
endif

EXTRA_DIST = \
	$(ui_DATA)
