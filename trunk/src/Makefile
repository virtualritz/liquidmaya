LIQUIDVERSION = 1_3

ifeq ($(OS),linux)
	C++			= g++
	ALIBS = 
	ABINLIBS = 
	C++FLAGS	= -g -x c++ -MD -pipe -march=pentiumpro -LANG:std -DUNIX -D_LINUX -DLINUX -D_BOOL -D_ISOC9X_SOURCE -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -fexceptions -fno-gnu-keywords -fno-dollars-in-identifiers -Wall -W -Wno-comment -Wtraditional -Wundef -Wpointer-arith -Wcast-qual -Wwrite-strings -Winline -Woverloaded-virtual -Wno-sign-compare -Wno-parentheses
	#C++FLAGS	= -g -x c++ -MD -pipe -march=pentiumpro -LANG:std -DUNIX -D_LINUX -DLINUX -D_BOOL -D_ISOC9X_SOURCE -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -fexceptions -fno-gnu-keywords -fno-dollars-in-identifiers -fno-handle-signatures -Wall -W -Wno-comment -Wtraditional -Wundef -Wpointer-arith -Wcast-qual -Wwrite-strings -Winline -Woverloaded-virtual -Wno-sign-compare -Wno-parentheses
	LDFLAGS = 
	NO_TRANS_LINK = 
else
	C++			= CC
	ALIBS = -lC -lgen  -lmalloc
	ABINLIBS =
	C++FLAGS = -g -mips3 -n32 -LANG:std -D_IRIX -DUNIX
	C++FLAGSO = -Ofast=ip30 -mips3 -n32 -LANG:std
	MAYALIBAPP = 
	LDFLAGS = -n32
	NO_TRANS_LINK = -no_transitive_link
endif

INCLUDES	= -I. -I.. -I$(MAYA_LOCATION)/include -I../riboutput -I$(RMANTREE)/include -I$(ENTROPYHOME)/include -I../include

LIBS		= -L../lib/$(OS) -lribwrite -L$(MAYA_LOCATION)/lib -L$(RMANTREE)/lib -L$(ENTROPYHOME)/lib $(ALIBS)  -lOpenMayaUI -lOpenMayaAnim -lsloargs -lm -llkm -lzip -ltarget -lsleargs
PLUGINLIBS = -lOpenMaya -lOpenMayaRender
BINLIBS = $(ABINLIBS)
LD			= $(MAYA_LOCATION)/bin/mayald $(LDFLAGS)

.SUFFIXES: .cpp .o .so .c

default: liquidPlug liquidBin

LIQUIDMAINOBJS =  liquidRibObj.o \
		  liquidRibNode.o \
		  liquidRibData.o \
		  liquidGlobalHelpers.o \
		  liquidRibSurfaceData.o \
		  liquidRibNuCurveData.o \
		  liquidRibParticleData.o \
		  liquidRibMeshData.o \
		  liquidRibLocatorData.o \
		  liquidRibCoordData.o \
		  liquidRibLightData.o \
		  liquidRibHT.o \
		  liquidRibTranslator.o \
		  liquidGetSloInfo.o \
		  liquidGetAttr.o \
		  liquidAttachPrefAttribute.o \
		  liquidRibGenData.o \
		  liquidPreviewShader.o \
		  liquidRibSubdivisionData.o \
		  liquidMemory.o

liquidPlug : liquidPlug.o $(LIQUIDMAINOBJS)
	$(C++) -o ../bin/$(OS)/liquid$(LIQUIDVERSION).so -shared $@.o $(LIQUIDMAINOBJS) $(LIBS) $(PLUGLIBS) $(NO_TRANS_LINK) $(C++FLAGS)
	
liquidBin : liquidBin.o $(LIQUIDMAINOBJS)
	$(LD) -o ../bin/$(OS)/liquid$(LIQUIDVERSION) $@.o $(LIQUIDMAINOBJS) $(LIBS) $(BINLIBS) $(NO_TRANS_LINK) $(C++FLAGS) -DLIQUIDBIN
	
.cpp.o:
	$(C++) -c $(INCLUDES) $(C++FLAGS) $<

.cpp.i:
	$(C++) -E $(INCLUDES) $(C++FLAGS) $*.cc > $*.i

depend:
	makedepend $(INCLUDES) -I/usr/include/CC liquid.cc

clean:
	rm -rf *.so *.o *.d ii_files so_locations

# DO NOT DELETE
