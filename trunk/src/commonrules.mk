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
    	    	    liqRibParticleData.$(OBJEXT) \
    	    	    liqTokenPointer.$(OBJEXT) \
    	    	    liquidRibObj.$(OBJEXT) \
    	    	    liquidRibNode.$(OBJEXT) \
    	    	    liquidRibData.$(OBJEXT) \
    	    	    liquidGlobalHelpers.$(OBJEXT) \
    	    	    liquidRibSurfaceData.$(OBJEXT) \
    	    	    liquidRibNuCurveData.$(OBJEXT) \
    	    	    liquidRibMeshData.$(OBJEXT) \
    	    	    liquidRibLocatorData.$(OBJEXT) \
    	    	    liquidRibCoordData.$(OBJEXT) \
    	    	    liquidRibLightData.$(OBJEXT) \
    	    	    liquidRibHT.$(OBJEXT) \
    	    	    liquidRibTranslator.$(OBJEXT) \
    	    	    liquidGetSloInfo.$(OBJEXT) \
    	    	    liquidGetAttr.$(OBJEXT) \
    	    	    liquidAttachPrefAttribute.$(OBJEXT) \
    	    	    liquidRibGenData.$(OBJEXT) \
    	    	    liqPreviewShader.$(OBJEXT) \
    	    	    liquidRibSubdivisionData.$(OBJEXT) \
    	    	    liquidMemory.$(OBJEXT) \
    	    	    liquidProcessLauncher.$(OBJEXT)

LIQUIDOUTMAINOBJS = $(VPATH)/liqShader.$(OBJEXT) \
    	    	    $(VPATH)/liqRibParticleData.$(OBJEXT) \
    	    	    $(VPATH)/liqTokenPointer.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibObj.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibNode.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibData.$(OBJEXT) \
    	    	    $(VPATH)/liquidGlobalHelpers.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibSurfaceData.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibNuCurveData.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibMeshData.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibLocatorData.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibCoordData.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibLightData.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibHT.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibTranslator.$(OBJEXT) \
    	    	    $(VPATH)/liquidGetSloInfo.$(OBJEXT) \
    	    	    $(VPATH)/liquidGetAttr.$(OBJEXT) \
    	    	    $(VPATH)/liquidAttachPrefAttribute.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibGenData.$(OBJEXT) \
    	    	    $(VPATH)/liqPreviewShader.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibSubdivisionData.$(OBJEXT) \
    	    	    $(VPATH)/liquidMemory.$(OBJEXT) \
    	    	    $(VPATH)/liquidProcessLauncher.$(OBJEXT)

.SUFFIXES: .cpp .$(OBJEXT) .$(PLUGSUF) .c

LIQUIDPLUG = liquid.$(PLUGSUF)
LIQUIDBIN = liquid
LIQUIDLIB = libliquid.a

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
	$(CPP) $(LDFLAGS) -shared -o $(VPATH)/$(LIQUIDPLUG) $(VPATH)/liquidPlug.$(OBJEXT) $(VPATH)/$(LIQUIDLIB) $(LIBS)
	
$(LIQUIDBIN) : liquidBin.$(OBJEXT) $(LIQUIDLIB)
	@echo $@
	$(LD) -DLIQUIDBIN $(LDFLAGS) -o $(VPATH)/$(LIQUIDBIN) $(VPATH)/liquidBin.$(OBJEXT) $(VPATH)/$(LIQUIDLIB) $(LIQUIDBINLIBS)

$(LIQUIDLIB) : $(LIQUIDMAINOBJS)
	$(AR) $(VPATH)/$(LIQUIDLIB) $(LIQUIDOUTMAINOBJS)

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
