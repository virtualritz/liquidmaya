DEPTH = ..
BIN_VERSION ?= debug
include $(DEPTH)/commondefs.mk
VPATH ?= $(VPATHDEBUG)

# -lefence
#-llkm -lzip -ltarget
LIQUIDBINLIBS =		$(LIQRMANLIBS) $(MAYALIBS) \
					-lOpenMayalib \
					-lOpenMayaFX \
					-lAnimSlice \
					-lDeformSlice \
					-lDynSlice \
					-lKinSlice \
					-lModelSlice \
					-lNurbsSlice \
					-lPolySlice \
					-lProjectSlice \
					-lImage \
					-lShared \
					-lTranslators \
					-lDataModel \
					-lRenderModel \
					-lNurbsEngine \
					-lDependEngine \
					-lCommandEngine \
					-lFoundation \
					-lStringCatalog \
					$(EXTRA_LIBS) \
					-lm

LIQUIDMAINOBJS = 	liqShader.$(OBJEXT) \
					liqAttachPrefAttribute.$(OBJEXT) \
					liqRibTranslator.$(OBJEXT) \
					liqRibParticleData.$(OBJEXT) \
					liqTokenPointer.$(OBJEXT) \
					liqWriteArchive.$(OBJEXT) \
					liqRibSurfaceData.$(OBJEXT) \
					liqPreviewShader.$(OBJEXT) \
					liqRibObj.$(OBJEXT) \
					liqRibNode.$(OBJEXT) \
					liqRibData.$(OBJEXT) \
					liqGlobalHelpers.$(OBJEXT) \
					liqRibNuCurveData.$(OBJEXT) \
					liqRibMeshData.$(OBJEXT) \
					liqRibLocatorData.$(OBJEXT) \
					liqRibClipPlaneData.$(OBJEXT) \
					liqRibCoordData.$(OBJEXT) \
					liqRibLightData.$(OBJEXT) \
					liqRibHT.$(OBJEXT) \
					liqGetSloInfo.$(OBJEXT) \
					liqGetAttr.$(OBJEXT) \
					liqRibGenData.$(OBJEXT) \
					liqRibSubdivisionData.$(OBJEXT) \
					liqRibMayaSubdivisionData.$(OBJEXT) \
					liqMemory.$(OBJEXT) \
					liqProcessLauncher.$(OBJEXT) \
					liqRenderer.$(OBJEXT) \
					liqExpression.$(OBJEXT) \
					liqNodeSwatch.$(OBJEXT) \
					liqSurfaceNode.$(OBJEXT)	\
					liqDisplacementNode.$(OBJEXT)	\
					liqVolumeNode.$(OBJEXT)  \
					liqRibboxNode.$(OBJEXT)  \
					liqLightNode.$(OBJEXT)  \
					liqLightNodeBehavior.$(OBJEXT)\
					liqCoordSysNode.$(OBJEXT) \
					liqGlobalsNode.$(OBJEXT) \
					liqBucket.$(OBJEXT) \
					liqMayaRenderView.$(OBJEXT) \
					liqJobList.$(OBJEXT)


LIQUIDOUTMAINOBJS := $(patsubst %,$(VPATH)/%,$(LIQUIDMAINOBJS))



.SUFFIXES: .cpp .$(OBJEXT) .$(PLUGSUF) .c

LIQUIDPLUG := $(VPATH)/liquid.$(PLUGSUF)
LIQUIDBIN  := $(VPATH)/liquid
LIQUIDLIB  := $(VPATH)/libliquid.a
LIQUIDDPY  := $(VPATH)/d_liqmaya.so

default: debug

#debug : EXTRAFLAGS=-g -DDEBUG
#debug : BIN_VERSION=debug
#release : BIN_VERSION=release

release newversion : $(VPATHRELEASE) $(LIQUIDPLUG) $(LIQUIDBIN) $(LIQUIDDPY)

debug : $(VPATHDEBUG) $(LIQUIDPLUG) $(LIQUIDBIN) $(LIQUIDDPY)

lib : $(VPATH) $(LIQUIDLIB)


liquidPlug : $(LIQUIDPLUG)

$(LIQUIDPLUG) : $(VPATH)/liquidPlug.$(OBJEXT) $(LIQUIDLIB)
	@echo $@
	$(CPP) $(LDFLAGS) -shared -o $(LIQUIDPLUG) $(VPATH)/liquidPlug.$(OBJEXT) $(LIQUIDLIB) $(LIBS)

$(LIQUIDBIN) : $(VPATH)/liquidBin.$(OBJEXT) $(LIQUIDLIB)
	@echo $@
	@$(LD) -DLIQUIDBIN $(LDFLAGS) -o $(LIQUIDBIN) $(VPATH)/liquidBin.$(OBJEXT) $(LIQUIDLIB) $(LIQUIDBINLIBS)

$(LIQUIDLIB) : $(LIQUIDOUTMAINOBJS)
	@$(AR) $(LIQUIDLIB) $(LIQUIDOUTMAINOBJS)

$(LIQUIDDPY) : $(VPATH)/$(LIQDISPLAYOBJS)
	@echo $@
	$(CPP) -shared $(VPATH)/$(LIQDISPLAYOBJS) -L$(LIQRMANPATH)/lib $(LIQRMANLIBS) -o $(LIQUIDDPY)

$(LIQUIDOUTMAINOBJS) : ../include/liquid.h

$(VPATH)/%.$(OBJEXT) : %.cpp
	@echo $@
	$(CPP) -c $(INCLUDES) $(CPPFLAGS) -o $@ $<

liqRibTranslator.o: ../include/liqRenderScript.h

.cpp.$(OBJEXT):
	@echo $@
	$(CPP) -c $(INCLUDES) $(CPPFLAGS) -o $(VPATH)/$@ $<

.cpp.i:
	@$(CPP) -E $(INCLUDES) $(CPPFLAGS) $*.cc > $(VPATH)/$*.i

depend:
	makedepend $(INCLUDES) -I/usr/include/CC liquid.cc


include $(DEPTH)/commonrules.mk
