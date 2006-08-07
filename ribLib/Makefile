CPPFLAGS += -DHAVE_ZLIB
RIBLIB := ../bin/linux32/librib.a
RIBLIBOBJS = ri.o\
	riInterface.o\
	ribOut.o\
	variable.o\
	error.o

all : $(RIBLIBOBJS) $(RIBLIB)

$(RIBLIB) :
	@mkdir -p ../bin/linux32
	@$(AR) cqs $(RIBLIB) $(RIBLIBOBJS)

.cpp.o :
	$(CPP) -c $(INCLUDES) $(CPPFLAGS) -o $@ $<

clean :
	rm -f *.o
	rm -f $(RIBLIB)
