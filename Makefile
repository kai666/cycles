CC=		clang
#DEBUG=		-g
CFLAGS=		${DEBUG} -Wall
LDFLAGS=	${DEBUG}

CYCLES_OBJS=	cycles.o
TARGETS=	cycles

DEB=		cycles-0.1.deb


all:		${TARGETS}

cycles:		${CYCLES_OBJS}

clean:
	rm -f ${CYCLES_OBJS} ${TARGETS}
	rm -rf deb ${DEB}

INSTALL=	install
INSTALL_EXE=	${INSTALL} -m 0755
INSTALL_SUID=	${INSTALL} -m 4755
INSTALL_DIR=	${INSTALL} -m 0755 -d
INSTALL_DATA=	${INSTALL} -m 0644

deb:		${DEB}

${DEB}:
	mkdir -m 0755 -p deb/usr/bin
	mkdir -m 0755 -p deb/DEBIAN
	${INSTALL_EXE} cycles deb/usr/bin
	${INSTALL_DATA} DEBIAN.control deb/DEBIAN/control
	fakeroot dpkg-deb --build deb $@
