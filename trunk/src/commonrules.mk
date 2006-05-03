DEPTH = ..
include $(DEPTH)/commondefs.mk

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



LIQUIDOUTMAINOBJS = $(VPATH)/liqShader.$(OBJEXT) \
					$(VPATH)/liqAttachPrefAttribute.$(OBJEXT) \
					$(VPATH)/liqRibParticleData.$(OBJEXT) \
					$(VPATH)/liqTokenPointer.$(OBJEXT) \
					$(VPATH)/liqWriteArchive.$(OBJEXT) \
					$(VPATH)/liqPreviewShader.$(OBJEXT) \
					$(VPATH)/liqRibSurfaceData.$(OBJEXT) \
					$(VPATH)/liqRibTranslator.$(OBJEXT) \
					$(VPATH)/liqRibObj.$(OBJEXT) \
					$(VPATH)/liqRibNode.$(OBJEXT) \
					$(VPATH)/liqRibData.$(OBJEXT) \
					$(VPATH)/liqGlobalHelpers.$(OBJEXT) \
					$(VPATH)/liqRibNuCurveData.$(OBJEXT) \
					$(VPATH)/liqRibMeshData.$(OBJEXT) \
					$(VPATH)/liqRibLocatorData.$(OBJEXT) \
					$(VPATH)/liqRibClipPlaneData.$(OBJEXT) \
					$(VPATH)/liqRibCoordData.$(OBJEXT) \
					$(VPATH)/liqRibLightData.$(OBJEXT) \
					$(VPATH)/liqRibHT.$(OBJEXT) \
					$(VPATH)/liqGetSloInfo.$(OBJEXT) \
					$(VPATH)/liqGetAttr.$(OBJEXT) \
					$(VPATH)/liqRibGenData.$(OBJEXT) \
					$(VPATH)/liqRibSubdivisionData.$(OBJEXT) \
					$(VPATH)/liqRibMayaSubdivisionData.$(OBJEXT) \
					$(VPATH)/liqMemory.$(OBJEXT) \
					$(VPATH)/liqProcessLauncher.$(OBJEXT) \
					$(VPATH)/liqRenderer.$(OBJEXT) \
					$(VPATH)/liqExpression.$(OBJEXT)  \
					$(VPATH)/liqNodeSwatch.$(OBJEXT)  \
					$(VPATH)/liqSurfaceNode.$(OBJEXT) \
					$(VPATH)/liqDisplacementNode.$(OBJEXT)	\
					$(VPATH)/liqVolumeNode.$(OBJEXT)  \
					$(VPATH)/liqRibboxNode.$(OBJEXT)  \
					$(VPATH)/liqLightNode.$(OBJEXT)  \
					$(VPATH)/liqLightNodeBehavior.$(OBJEXT)\
					$(VPATH)/liqCoordSysNode.$(OBJEXT) \
					$(VPATH)/liqGlobalsNode.$(OBJEXT) \
					$(VPATH)/liqBucket.$(OBJEXT) \
					$(VPATH)/liqMayaRenderView.$(OBJEXT) \
					$(VPATH)/liqJobList.$(OBJEXT)

ifeq ($(LIQRMAN),3delight)
	LIQDISPLAYOBJS = liqMayaDisplayDriver3Delight.$(OBJEXT)
else
	ifeq ($(LIQRMAN),pixie)
		LIQDISPLAYOBJS = liqMayaDisplayDriverPixie.$(OBJEXT)
	else
		LIQDISPLAYOBJS = liqMayaDisplayDriver.$(OBJEXT)
	endif
endif

.SUFFIXES: .cpp .$(OBJEXT) .$(PLUGSUF) .c

LIQUIDPLUG = liquid.$(PLUGSUF)
LIQUIDBIN  = liquid
LIQUIDLIB  = libliquid.a
LIQUIDDPY  = d_liqmaya.so

default: $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN) lib $(LIQUIDDPY)

debug : $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN) $(LIQUIDDPY)

newversion : $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN) $(LIQUIDDPY)

release : $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN) $(LIQUIDDPY)

dist : distdir release
	@$(CP) $(VPATH)/$(LIQUIDPLUG) $(DIST_DIR)/$(LIQUIDPLUG)

lib : $(LIQUIDLIB)

$(VPATH) :
	@sh -c "if( test -d $(VPATH) ); then \
		echo '$(VPATH) already exists'; \
	else \
		echo 'creating $(VPATH)'; \
		mkdir -p $(VPATH); \
	fi;"

liquidPlug : $(LIQUIDPLUG)

$(LIQUIDPLUG) : liquidPlug.$(OBJEXT) $(LIQUIDLIB)
	@echo $@
	@$(CPP) $(LDFLAGS) -shared -o $(VPATH)/$(LIQUIDPLUG) $(VPATH)/liquidPlug.$(OBJEXT) $(VPATH)/$(LIQUIDLIB) $(LIBS)

$(LIQUIDBIN) : liquidBin.$(OBJEXT) $(LIQUIDLIB)
	@echo $@
	@$(LD) -DLIQUIDBIN $(LDFLAGS) -o $(VPATH)/$(LIQUIDBIN) $(VPATH)/liquidBin.$(OBJEXT) $(VPATH)/$(LIQUIDLIB) $(LIQUIDBINLIBS)

$(LIQUIDLIB) : $(LIQUIDMAINOBJS)
	@$(AR) $(VPATH)/$(LIQUIDLIB) $(LIQUIDOUTMAINOBJS)

$(LIQUIDDPY) : $(LIQDISPLAYOBJS)
	@echo $@
	$(CPP) -shared $(VPATH)/$(LIQDISPLAYOBJS) -L$(LIQRMANPATH)/lib $(LIQRMANLIBS) -o $(VPATH)/$(LIQUIDDPY)


$(LIQUIDMAINOBJS) : ../include/liquid.h
liqRibTranslator.o: ../include/liqRenderScript.h

.cpp.$(OBJEXT):
	@echo $@
	@$(CPP) -c $(INCLUDES) $(CPPFLAGS) -o $(VPATH)/$@ $<

.cpp.i:
	@$(CPP) -E $(INCLUDES) $(CPPFLAGS) $*.cc > $(VPATH)/$*.i

depend:
	makedepend $(INCLUDES) -I/usr/include/CC liquid.cc

clean:
	rm -rf $(VPATH)/*.$(PLUGSUF) $(VPATH)/*.$(OBJEXT) $(VPATH)/*.d $(VPATH)/ii_files $(VPATH)/so_locations

include $(DEPTH)/commonrules.mk
