# ----------------------------------------------------------------------
# Makefile for thermistor example.
# ----------------------------------------------------------------------


include ../../Makefile.os


EXAMPLENAME=	thermistor


PROGS=		$(EXAMPLENAME)$(EXESUFFIX)
OBJS1=		$(EXAMPLENAME).o


ALL=		$(PROGS)


LIBOPEN8055=		../../libopen8055/libopen8055.a


CC=			gcc
CFLAGS+=	-O0 -g -Wall -I../../include
LDFLAGS+=	-static 
ifeq ($(OSFAMILY), Unix)
LIBS=		-lm -lusb -lpthread
else ifeq ($(OSFAMILY), Windows)
LIBS=		-lsetupapi -lrpcrt4
endif



all:	$(ALL)


clean:
	rm -f $(PROGS) $(OBJS1)


$(EXAMPLENAME)$(EXESUFFIX):	$(EXAMPLENAME).o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBOPEN8055) $(LIBS)


$(EXAMPLENAME).o:	$(EXAMPLENAME).c


