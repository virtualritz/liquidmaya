SHELL	    	= sh
VPATH		= ../bin/liquid$(LIQUIDVERSION)/$(MAYA_VERSION)/$(VBIN)
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
    	    	    liqTokenPointer.$(OBJEXT) \
    	    	    liquidRibObj.$(OBJEXT) \
    	    	    liquidRibNode.$(OBJEXT) \
    	    	    liquidRibData.$(OBJEXT) \
    	    	    liquidGlobalHelpers.$(OBJEXT) \
    	    	    liquidRibSurfaceData.$(OBJEXT) \
    	    	    liquidRibNuCurveData.$(OBJEXT) \
    	    	    liquidRibParticleData.$(OBJEXT) \
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
    	    	    liquidPreviewShader.$(OBJEXT) \
    	    	    liquidRibSubdivisionData.$(OBJEXT) \
    	    	    liquidMemory.$(OBJEXT)

LIQUIDOUTMAINOBJS = $(VPATH)/liqShader.$(OBJEXT) \
    	    	    $(VPATH)/liqTokenPointer.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibObj.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibNode.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibData.$(OBJEXT) \
    	    	    $(VPATH)/liquidGlobalHelpers.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibSurfaceData.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibNuCurveData.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibParticleData.$(OBJEXT) \
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
    	    	    $(VPATH)/liquidPreviewShader.$(OBJEXT) \
    	    	    $(VPATH)/liquidRibSubdivisionData.$(OBJEXT) \
    	    	    $(VPATH)/liquidMemory.$(OBJEXT)

.SUFFIXES: .cpp .$(OBJEXT) .$(PLUGSUF) .c

LIQUIDPLUG = liquid$(LIQUIDVERSION).$(PLUGSUF)
LIQUIDBIN = liquid$(LIQUIDVERSION)

default: $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN)

debug : $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN)

newversion : $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN)

release : $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN)

$(VPATH) :
	@sh -c "if( test -d $(VPATH) ); then \
	    echo '$(VPATH) already exist'; \
	else \
	    echo 'creating $(VPATH)'; \
	    mkdir -p $(VPATH); \
	fi;"

liquidPlug : $(LIQUIDPLUG)

$(LIQUIDPLUG) : liquidPlug.$(OBJEXT) $(LIQUIDMAINOBJS)
	@echo $@
	@$(CPP) $(LDFLAGS) -shared -o $(VPATH)/$(LIQUIDPLUG) $(VPATH)/liquidPlug.$(OBJEXT) $(LIQUIDOUTMAINOBJS) $(LIBS)
	
$(LIQUIDBIN) : liquidBin.$(OBJEXT) $(LIQUIDMAINOBJS)
	@echo $@
	@$(LD) -DLIQUIDBIN $(LDFLAGS) -o $(VPATH)/$(LIQUIDBIN) $(VPATH)/liquidBin.$(OBJEXT) $(LIQUIDOUTMAINOBJS) $(LIQUIDBINLIBS)


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
