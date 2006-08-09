
DEPTH = ..
BIN_VERSION ?= debug
include $(DEPTH)/commondefs.mk
VPATH ?= $(VPATHDEBUG)

CPPFLAGS += -DHAVE_ZLIB
RIBLIB := ../bin/linux32/librib.a
RIBLIBOBJS = ri.o\
	riInterface.o\
	ribOut.o\
	variable.o\
	error.o

all : $(RIBLIB)

$(RIBLIB) : $(RIBLIBOBJS)
	@mkdir -p ../bin/linux32/
	$(AR) $(RIBLIB) $(RIBLIBOBJS)

.cpp.o :
	$(CPP) -c $(INCLUDES) $(CPPFLAGS) -o $@ $<

clean :
	rm -f *.o
	rm -f $(RIBLIB)
