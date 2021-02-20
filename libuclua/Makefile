
LIB=	uclua
SHLIB_MAJOR=	0
VERSION_MAP=	${.CURDIR}/lib${LIB}.ver

SRCS=	luclua.c luclua_ucl.c

CFLAGS+=	-I${LOCALBASE}/include/lua53 -fsanitize=undefined

LDADD=	-L${LOCALBASE}/lib -llua-5.3
LDADD+=	-lucl

.include <bsd.lib.mk>
