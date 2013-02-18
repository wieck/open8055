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
else ifeq ($(findstring MINGW32, $(UNAME)), MINGW32)
	OSFAMILY=Windows
	EXESUFFIX=.exe
else
fatal:
	$(error Operating System $(UNAME) not supported (yet))
endif

