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

LIQUIDMAINOBJS =  liqShader.$(OBJEXT) \
                  liqAttachPrefAttribute.$(OBJEXT) \
                  liqEntropyRenderer.$(OBJEXT) \
                  liqPrmanRenderer.$(OBJEXT) \
                  liqAqsisRenderer.$(OBJEXT) \
                  liqDelightRenderer.$(OBJEXT) \
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
                  liqRenderer.$(OBJEXT)

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
                    $(VPATH)/liqRibTranslator.$(OBJEXT) \
                    $(VPATH)/liqRibObj.$(OBJEXT) \
                    $(VPATH)/liqRibNode.$(OBJEXT) \
                    $(VPATH)/liqRibData.$(OBJEXT) \
                    $(VPATH)/liqGlobalHelpers.$(OBJEXT) \
                    $(VPATH)/liqRibNuCurveData.$(OBJEXT) \
                    $(VPATH)/liqRibMeshData.$(OBJEXT) \
                    $(VPATH)/liqRibLocatorData.$(OBJEXT) \
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
                    $(VPATH)/liqRenderer.$(OBJEXT)

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
