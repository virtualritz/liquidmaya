SHELL	    	= sh
VPATH		= ../liquid$(LIQUIDVERSION)/bin/$(VBIN)/$(MAYA_VERSION)
CPPFLAGS     	= $(LOCFLAGS) $(LIQRMANFLAGS) $(WARNFLAGS) $(EXTRAFLAGS) $(NO_TRANS_LINK)
INCLUDES	= -I. -I.. -I$(MAYA_LOCATION)/include -I$(LIQRMANPATH)/include -I../include
LDFLAGS     	= $(CPPFLAGS) -L$(MAYA_LOCATION)/lib -L$(LIQRMANPATH)/lib
MAYALIBS  	= -lOpenMaya -lOpenMayaRender  -lOpenMayaUI -lOpenMayaAnim
LIBS		= $(LIQRMANLIBS) $(MAYALIBS) $(EXTRA_LIBS) -lm
# -lefence
#-llkm -lzip -ltarget
LIQUIDBINLIBS    =  $(LIQRMANLIBS) $(MAYALIBS) \
          -lOpenMayalib \
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

LIQUIDMAINOBJS =    liqShader.$(OBJEXT) \
    	    	    liqAttachPrefAttribute.$(OBJEXT) \
    	    	    liqEntropyRenderer.$(OBJEXT) \
    	    	    liqPrmanRenderer.$(OBJEXT) \
    	    	    liqAqsisRenderer.$(OBJEXT) \
    	    	    liqDelightRenderer.$(OBJEXT) \
    	    	    liquidRibTranslator.$(OBJEXT) \
    	    	    liqRibParticleData.$(OBJEXT) \
    	    	    liqTokenPointer.$(OBJEXT) \
		    liqWriteArchive.$(OBJEXT) \
    	    	    liqRibSurfaceData.$(OBJEXT) \
    	    	    liqPreviewShader.$(OBJEXT) \
    	    	    liquidRibObj.$(OBJEXT) \
    	    	    liquidRibNode.$(OBJEXT) \
    	    	    liquidRibData.$(OBJEXT) \
    	    	    liqGlobalHelpers.$(OBJEXT) \
    	    	    liquidRibNuCurveData.$(OBJEXT) \
    	    	    liquidRibMeshData.$(OBJEXT) \
    	    	    liquidRibLocatorData.$(OBJEXT) \
    	    	    liqRibCoordData.$(OBJEXT) \
    	    	    liquidRibLightData.$(OBJEXT) \
    	    	    liquidRibHT.$(OBJEXT) \
    	    	    liqGetSloInfo.$(OBJEXT) \
    	    	    liqGetAttr.$(OBJEXT) \
    	    	    liquidRibGenData.$(OBJEXT) \
    	    	    liquidRibSubdivisionData.$(OBJEXT) \
    	    	    liqMemory.$(OBJEXT) \
    	    	    liqProcessLauncher.$(OBJEXT)

LIQUIDOUTMAINOBJS = $(VPATH)/liqShader.$(OBJEXT) \
    	    	    $(VPATH)/liqAttachPrefAttribute.$(OBJEXT) \
    	    	    $(VPATH)/liqEntropyRenderer.$(OBJEXT) \
    	    	    $(VPATH)/liqPrmanRenderer.$(OBJEXT) \
    	    	    $(VPATH)/liqAqsisRenderer.$(OBJEXT) \
    	    	    $(VPATH)/liqDelightRenderer.$(OBJEXT) \
    	    	    $(VPATH)/liqRibParticleData.$(OBJEXT) \
    	    	    $(VPATH)/liqTokenPointer.$(OBJEXT) \
		    $(VPATH)/liqWriteArchive.$(OBJEXT) \
    	    	    $(VPATH)/liqPreviewShader.$(OBJEXT) \
    	    	    $(VPATH)/liqRibSurfaceData.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibTranslator.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibObj.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibNode.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibData.$(OBJEXT) \
    	    	    $(VPATH)/liqGlobalHelpers.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibNuCurveData.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibMeshData.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibLocatorData.$(OBJEXT) \
    	    	    $(VPATH)/liqRibCoordData.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibLightData.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibHT.$(OBJEXT) \
    	    	    $(VPATH)/liqGetSloInfo.$(OBJEXT) \
    	    	    $(VPATH)/liqGetAttr.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibGenData.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibSubdivisionData.$(OBJEXT) \
    	    	    $(VPATH)/liqMemory.$(OBJEXT) \
    	    	    $(VPATH)/liqProcessLauncher.$(OBJEXT)

.SUFFIXES: .cpp .$(OBJEXT) .$(PLUGSUF) .c

LIQUIDPLUG = liquid.$(PLUGSUF)
LIQUIDBIN  = liquid
LIQUIDLIB  = libliquid.a

default: $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN) lib

debug : $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN)

newversion : $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN)

release : $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN)

lib : $(LIQUIDLIB)

$(VPATH) :
	@sh -c "if( test -d $(VPATH) ); then \
	    echo '$(VPATH) already exist'; \
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

$(LIQUIDMAINOBJS) : ../include/liquid.h

.cpp.$(OBJEXT):
	@echo $@
	@$(CPP) -c $(INCLUDES) $(CPPFLAGS) -o $(VPATH)/$@ $<

.cpp.i:
	$(CPP) -E $(INCLUDES) $(CPPFLAGS) $*.cc > $(VPATH)/$*.i

depend:
	makedepend $(INCLUDES) -I/usr/include/CC liquid.cc

clean:
	rm -rf $(VPATH)/*.$(PLUGSUF) $(VPATH)/*.$(OBJEXT) $(VPATH)/*.d $(VPATH)/ii_files $(VPATH)/so_locations
