# ----------------------------------------------------------------------
# Makefile.os
#
#	Operating system detection.
# ----------------------------------------------------------------------

UNAME=	$(shell uname)

ifeq ($(UNAME), FreeBSD)
	OSFAMILY=Unix
	EXESUFFIX=
else ifeq ($(UNAME), Linux)
	OSFAMILY=Unix
	EXESUFFIX=
else
fatal:
	$(error Operating System $(UNAME) not supported (yet))
endif

