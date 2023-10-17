CC=		clang
#DEBUG=		-g
CFLAGS=		${DEBUG} -Wall
LDFLAGS=	${DEBUG}

CYCLES_OBJS=	cycles.o
TARGETS=	cycles

all:		${TARGETS}

cycles:		${CYCLES_OBJS}

clean:
	rm -f ${CYCLES_OBJS} ${TARGETS}
