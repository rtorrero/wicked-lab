
CLEANFILES			= *~
MAINTAINERCLEANFILES		= Makefile.in

INCLUDES			= -I$(top_srcdir)	\
				  -I$(top_srcdir)/include

wicked_lib_LTLIBRARIES		= libwicked.la
noinst_LTLIBRARIES		= libwicked-dbus-objects.la

libwicked_la_CFLAGS		= $(LIBNL_CFLAGS)	\
				  $(LIBDBUS_CFLAGS)	\
				  $(LIBGCRYPT_CFLAGS)

libwicked_la_CPPFLAGS		= -DWICKED_CONFIGDIR=\"$(wicked_configdir)\"
libwicked_la_LDFLAGS		= -lm			\
				  $(LIBDL_LIBS)		\
				  $(LIBNL_LIBS)		\
				  $(LIBANL_LIBS)	\
				  $(LIBDBUS_LIBS)	\
				  $(LIBGCRYPT_LIBS)	\
				  $(LIBWICKED_LTLINK_VERSION)
libwicked_la_LIBADD		= libwicked-dbus-objects.la
libwicked_la_SOURCES		= \
	address.c		\
	arp.c			\
	async-resolver.c	\
	bonding.c		\
	bridge.c		\
	buffer.c		\
	calls.c			\
	capture.c		\
	config.c		\
	dbus-client.c		\
	dbus-common.c		\
	dbus-connection.c	\
	dbus-dict.c		\
	dbus-errors.c		\
	dbus-introspect.c	\
	dbus-message.c		\
	dbus-object.c		\
	dbus-server.c		\
	dbus-xml.c		\
	dhcp-lease.c		\
	errors.c		\
	ethernet.c		\
	extension.c		\
	firmware.c		\
	fsm.c			\
	fsm-policy.c		\
	ifconfig.c		\
	ifevent.c		\
	iflist.c		\
	interface.c		\
	ipv4.c			\
	ipv6.c			\
	kernel.c		\
	leasefile.c		\
	logging.c		\
	md5sum.c		\
	modem-manager.c		\
	names.c			\
	netinfo.c		\
	nis.c			\
	openvpn.c		\
	ppp.c			\
	process.c		\
	resolver.c		\
	rfkill.c		\
	secret.c		\
	socket.c		\
	state.c			\
	sysconfig.c		\
	sysfs.c			\
	timer.c			\
	update.c		\
	util.c			\
	vlan.c			\
	wireless.c		\
	wpa-supplicant.c	\
	xml.c			\
	xml-reader.c		\
	xml-schema.c		\
	xml-writer.c		\
	xpath.c			\
	xpath-fmt.c

libwicked_dbus_objects_la_CFLAGS	= $(libwicked_la_CFLAGS)
libwicked_dbus_objects_la_CPPFLAGS	= $(libwicked_la_CPPFLAGS)
libwicked_dbus_objects_la_SOURCES	= \
	dbus-objects/addrconf.c	\
	dbus-objects/bonding.c	\
	dbus-objects/bridge.c	\
	dbus-objects/ethernet.c	\
	dbus-objects/interface.c\
	dbus-objects/ipv4.c	\
	dbus-objects/ipv6.c	\
	dbus-objects/misc.c	\
	dbus-objects/model.c	\
	dbus-objects/modem.c	\
	dbus-objects/naming.c	\
	dbus-objects/openvpn.c	\
	dbus-objects/ppp.c	\
	dbus-objects/state.c	\
	dbus-objects/tun.c	\
	dbus-objects/vlan.c	\
	dbus-objects/wireless.c

EXTRA_DIST			= \
	appconfig.h		\
	buffer.h		\
	dbus-common.h		\
	dbus-connection.h	\
	dbus-dict.h		\
	dbus-object.h		\
	dbus-objects/model.h	\
	dbus-server.h		\
	debug.h			\
	ipv6_priv.h		\
	kernel.h		\
	modem-manager.h		\
	netinfo_priv.h		\
	process.h		\
	socket_priv.h		\
	sysfs.h			\
	util_priv.h		\
	wireless_priv.h		\
	wpa-supplicant.h	\
	xml-schema.h

install-data-hook:
	if test "$(wicked_libdir)" != "$(wicked_devellibdir)" ; then     \
		rm -f "$(DESTDIR)$(wicked_libdir)/libwicked.so"        ; \
		$(mkinstalldirs) "$(DESTDIR)$(wicked_devellibdir)"     ; \
		$(LN_S) -f "$(wicked_libdir)/libwicked.so.$(LIBWICKED_SOFILE_VERSION)" \
			"$(DESTDIR)$(wicked_devellibdir)/libwicked.so" ; \
	fi

uninstall-hook:
	if test "$(wicked_libdir)" != "$(wicked_devellibdir)" ; then     \
		rm -f "$(DESTDIR)$(wicked_devellibdir)/libwicked.so"   ; \
	fi

# vim: ai