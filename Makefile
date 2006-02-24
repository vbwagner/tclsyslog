VERSION=1.1
# This is root of installation tree
PREFIX=/usr/local
CC=gcc
#
# Don't forget to change if your CC is not gcc
#
CFLAGS=-Wall -fPIC
LDFLAGS=-shared
# No need to link with libtcl8.0 on ELF system. Your setup might be
# different
LOADLIBES=
# This is where package would be installed
LIBDIR=${PREFIX}/lib
# On my Debian system this would be
# LIBDIR=/usr/local/lib/site-tcl
# On Debian Linux this would be
# MANSECTION=3
# MANSUFFIX=3tcl
MANSECTION=n
MANSUFFIX=n
MANDIR=${PREFIX}/man/man${MANSECTION}
# install program. Must be GNU install compatible. install-sh from the
# tcl distribution is good replacement if your install is not GNU
# compatible
INSTALL=/usr/bin/install
# End of configuration settings
all: libsyslog.so.${VERSION} pkgIndex.tcl

libsyslog.so.${VERSION}: tclsyslog.o
	gcc ${LDFLAGS} -o libsyslog.so.${VERSION} -DVERSION=\"${VERSION}\" tclsyslog.o ${LOADLIBES}

tclsyslog.o: tclsyslog.c
	${CC} ${CFLAGS} ${INCLUDES} -DVERSION=\"${VERSION}\" -c tclsyslog.c

pkgIndex.tcl: libsyslog.so.${VERSION}
	echo 'package ifneeded Syslog ${VERSION} [list tclPkgSetup $$dir Syslog ${VERSION} {{libsyslog.so.${VERSION} load {syslog}}}]' >pkgIndex.tcl
clean:
	-rm libsyslog.so.${VERSION} pkgIndex.tcl *~ *.o
install:	
	${INSTALL} -m 755 -d ${LIBDIR}/syslog
	${INSTALL} -m 755 -c libsyslog.so.${VERSION} ${LIBDIR}/syslog 
	${INSTALL} -m 644 -c pkgIndex.tcl ${LIBDIR}/syslog
	${INSTALL} -m 644 -c syslog.n ${MANDIR}/syslog.${MANSUFFIX}

