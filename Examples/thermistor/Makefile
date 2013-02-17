# ----------------------------------------------------------------------
# Makefile for thermistor example.
# ----------------------------------------------------------------------


include ../../Makefile.os


EXAMPLENAME=	thermistor


PROGS=		$(EXAMPLENAME)$(EXESUFFIX)
OBJS1=		$(EXAMPLENAME).o


ALL=		$(PROGS)


CFLAGS+=	-O0 -g -Wall -I../../include
LDFLAGS+=	-static -L../../libopen8055 -lopen8055


all:	$(ALL)


clean:
	rm -f $(PROGS) $(OBJS1)


$(EXAMPLENAME)$(EXESUFFIX):	$(EXAMPLENAME).o


$(EXAMPLENAME).o:	$(EXAMPLENAME).c


