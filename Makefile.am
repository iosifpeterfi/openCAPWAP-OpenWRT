ACLOCAL_AMFLAGS=-I m4

AM_CFLAGS = -I$(srcdir)/include -D_REENTRANT
AM_CFLAGS += -DOPENSSL_NO_KRB5

# Capwap Debugging
AM_CFLAGS += -DCW_DEBUGGING
#AM_CFLAGS += -DWRITE_STD_OUTPUT
AM_CFLAGS += -DSOFTMAC
AM_CFLAGS += -Wall

bin_PROGRAMS = AC WTP WUA

noinst_LTLIBRARIES = libcapwap.la
libcapwap_la_SOURCES = \
	CWLog.c \
	CWCommon.c \
	CWConfigFile.c \
	CWErrorHandling.c \
	CWList.c \
	CWSafeList.c \
	CWNetwork.c \
	CWProtocol.c \
	CWRandom.c \
	CWStevens.c \
	CWThread.c \
	CWBinding.c \
	CWVendorPayloadsAC.c \
	CWVendorPayloadsWTP.c \
	timerlib.c

if USE_SSL
libcapwap_la_SOURCES += \
	CWSecurity.c \
	CWOpenSSLBio.c
endif

libcapwap_la_CFLAGS = ${OPENSSL_INCLUDES}
libcapwap_la_LIBADD = ${OPENSSL_LDFLAGS} ${OPENSSL_LIBS}

AC_SOURCES = \
	AC.c \
	ACConfigFile.c \
	ACMainLoop.c \
	ACDiscoveryState.c \
	ACJoinState.c \
	ACConfigureState.c \
	ACDataCheckState.c \
	ACRunState.c \
	ACProtocol_User.c \
	ACRetransmission.c \
	ACMultiHomedSocket.c \
	ACProtocol.c \
	ACBinding.c \
	ACInterface.c \
	ACSettingsFile.c \
	tap.c \
	ACipcHostapd.c
AC_LDADD = libcapwap.la

WTP_SOURCES = \
	WTP.c \
	WTPipcHostapd.c \
	WTPmacFrameReceive.c \
	WTPFreqStatsReceive.c \
	WTPStatsReceive.c \
	WTPConfigFile.c \
	WTPProtocol.c \
	WTPProtocol_User.c \
	WTPDiscoveryState.c \
	WTPJoinState.c \
	WTPConfigureState.c \
	WTPDataCheckState.c \
	WTPRunState.c \
	WTPRunStateCheck.c \
	WTPRetransmission.c \
	WTPSulkingState.c \
	WTPBinding.c \
	WTPmacDriverInteraction.c \
	WTPSettingsFile.c
WTP_LDADD = libcapwap.la

WUA_SOURCES = WUA.c
