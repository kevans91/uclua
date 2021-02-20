
SUBDIR+=	libuclua
SUBDIR+=	src

SUBDIR_DEPEND_src=	libuclua
SUBDIR_PARALLEL=	yes

.include <bsd.subdir.mk>
