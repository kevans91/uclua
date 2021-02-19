
LIB=	uclua
SHLIB_MAJOR=	0

SRCS=	luclua.c

LOCALBASE?=	/usr/local

CFLAGS+=	-I${LOCALBASE}/include/lua53

LDADD=	-L${LOCALBASE}/lib -llua-5.3

.include <bsd.lib.mk>
