SHELL		= sh
VPATH		= ../bin/$(VBIN)/$(MAYA_VERSION)/$(LIQRMAN)
SETUPPATH	= ../W32Installer
CPPFLAGS	= $(LOCFLAGS) $(LIQRMANFLAGS) $(WARNFLAGS) $(EXTRAFLAGS) $(NO_TRANS_LINK)
INCLUDES	= -I. -I.. -I../include -I"$(MAYA_LOCATION)/include" -I"$(LIQRMANPATH)/include"
LDFLAGS		= $(CPPFLAGS) $(LIBPATHFLAG)"$(MAYA_LOCATION)/lib" $(LIBPATHFLAG)"$(LIQRMANPATH)/lib" $(LIBPATHFLAG)/usr/lib
MAYALIBS	= $(LIBFLAG)OpenMaya$(LIBSUF) $(LIBFLAG)OpenMayaRender$(LIBSUF) $(LIBFLAG)OpenMayaUI$(LIBSUF) $(LIBFLAG)OpenMayaAnim$(LIBSUF) $(LIBFLAG)OpenMayaFX$(LIBSUF)
LIBS		= $(MAYALIBS) $(EXTRALIBS)
#$(LIQRMANLIBS)
# -lefence
#-llkm -lzip -ltarget
LIQUIDBINLIBS =		$(MAYALIBS) \
					$(LIBFLAG)OpenMaya$(LIBSUF) \
					$(LIBFLAG)OpenMayaFX$(LIBSUF) \
					$(LIBFLAG)Foundation$(LIBSUF) \
					$(EXTRALIBS)
#					$(LIBFLAG)AnimSlice$(LIBSUF) \
#					$(LIBFLAG)DeformSlice$(LIBSUF) \
#					$(LIBFLAG)DynSlice$(LIBSUF) \
#					$(LIBFLAG)KinSlice$(LIBSUF) \
#					$(LIBFLAG)ModelSlice$(LIBSUF) \
#					$(LIBFLAG)NurbsSlice$(LIBSUF) \
#					$(LIBFLAG)PolySlice$(LIBSUF) \
#					$(LIBFLAG)ProjectSlice$(LIBSUF) \
#					$(LIBFLAG)Image$(LIBSUF) \
#					$(LIBFLAG)Shared$(LIBSUF) \
#					$(LIBFLAG)Translators$(LIBSUF) \
#					$(LIBFLAG)DataModel$(LIBSUF) \
#					$(LIBFLAG)RenderModel$(LIBSUF) \
#					$(LIBFLAG)NurbsEngine$(LIBSUF) \
#					$(LIBFLAG)DependEngine$(LIBSUF) \
#					$(LIBFLAG)CommandEngine$(LIBSUF) \
#					$(LIQRMANLIBS)

LIQUIDMAINOBJS = 	liqShader$(OBJSUF) \
					liqAttachPrefAttribute$(OBJSUF) \
					liqRibTranslator$(OBJSUF) \
					liqRibParticleData$(OBJSUF) \
					liqTokenPointer$(OBJSUF) \
					liqWriteArchive$(OBJSUF) \
					liqRibSurfaceData$(OBJSUF) \
					liqPreviewShader$(OBJSUF) \
					liqRibObj$(OBJSUF) \
					liqRibNode$(OBJSUF) \
					liqRibData$(OBJSUF) \
					liqGlobalHelpers$(OBJSUF) \
					liqRibNuCurveData$(OBJSUF) \
					liqRibMeshData$(OBJSUF) \
					liqRibLocatorData$(OBJSUF) \
					liqRibCoordData$(OBJSUF) \
					liqRibLightData$(OBJSUF) \
					liqRibHT$(OBJSUF) \
					liqGetSloInfo$(OBJSUF) \
					liqGetAttr$(OBJSUF) \
					liqRibGenData$(OBJSUF) \
					liqRibSubdivisionData$(OBJSUF) \
					liqRibMayaSubdivisionData$(OBJSUF) \
					liqMemory$(OBJSUF) \
					liqProcessLauncher$(OBJSUF) \
					liqRenderer$(OBJSUF) \
					liqExpression$(OBJSUF)


LIQUIDOUTMAINOBJS = $(VPATH)/liqShader$(OBJSUF) \
					$(VPATH)/liqAttachPrefAttribute$(OBJSUF) \
					$(VPATH)/liqRibParticleData$(OBJSUF) \
					$(VPATH)/liqTokenPointer$(OBJSUF) \
					$(VPATH)/liqWriteArchive$(OBJSUF) \
					$(VPATH)/liqPreviewShader$(OBJSUF) \
					$(VPATH)/liqRibSurfaceData$(OBJSUF) \
					$(VPATH)/liqRibTranslator$(OBJSUF) \
					$(VPATH)/liqRibObj$(OBJSUF) \
					$(VPATH)/liqRibNode$(OBJSUF) \
					$(VPATH)/liqRibData$(OBJSUF) \
					$(VPATH)/liqGlobalHelpers$(OBJSUF) \
					$(VPATH)/liqRibNuCurveData$(OBJSUF) \
					$(VPATH)/liqRibMeshData$(OBJSUF) \
					$(VPATH)/liqRibLocatorData$(OBJSUF) \
					$(VPATH)/liqRibCoordData$(OBJSUF) \
					$(VPATH)/liqRibLightData$(OBJSUF) \
					$(VPATH)/liqRibHT$(OBJSUF) \
					$(VPATH)/liqGetSloInfo$(OBJSUF) \
					$(VPATH)/liqGetAttr$(OBJSUF) \
					$(VPATH)/liqRibGenData$(OBJSUF) \
					$(VPATH)/liqRibSubdivisionData$(OBJSUF) \
					$(VPATH)/liqRibMayaSubdivisionData$(OBJSUF) \
					$(VPATH)/liqMemory$(OBJSUF) \
					$(VPATH)/liqProcessLauncher$(OBJSUF) \
					$(VPATH)/liqRenderer$(OBJSUF) \
					$(VPATH)/liqExpression$(OBJSUF)

SETUPINCLUDES = $(SETUPPATH)/liquidversion.iss \
					$(SETUPPATH)/targetrenderer.iss

.SUFFIXES: .cpp $(OBJSUF) $(PLUGSUF) .c

LIQUIDPLUG  = liquid$(PLUGSUF)
LIQUIDBIN   = liquid$(BINSUF)
LIQUIDLIB   = libliquid$(LIBSUF)
LIQUIDSETUP = $(SETUPPATH)/Output/Liquid-$(LIQUIDSHORTVERSION)-$(LIQRMAN)-Maya-6-Setup.exe

default: $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN) lib

debug : $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN)

newversion : $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN)

release : $(VPATH) $(LIQUIDPLUG) $(LIQUIDBIN) $(LIQUIDSETUP)

lib : $(LIQUIDLIB)

$(VPATH) :
	@sh -c "if( test -d $(VPATH) ); then \
		echo '$(VPATH) already exists'; \
	else \
		echo 'Creating $(VPATH)'; \
		mkdir -p $(VPATH); \
	fi;"

liquidPlug : $(LIQUIDPLUG)

$(LIQUIDPLUG) : liquidPlug$(OBJSUF) $(LIQUIDMAINOBJS)
	@echo $@
	@link /OUT:$(VPATH)/$(LIQUIDPLUG) /OPT:ICF=4 /INCREMENTAL:NO /NOLOGO /LIBPATH:"$(MAYA_LOCATION)/lib" /LIBPATH:"$(LIQRMANPATH)/lib" /DLL /NODEFAULTLIB:libmmt.lib /NODEFAULTLIB:libircmt.lib /IMPLIB:$(VPATH)/liquid$(LIBSUF) /MACHINE:I386 $(LIQWINRMANLIBS) $(LIBS) $(VPATH)/liquidPlug$(OBJSUF) $(LIQUIDOUTMAINOBJS)

$(LIQUIDBIN) : liquidBin$(OBJSUF) $(LIQUIDMAINOBJS)
	@echo $@
	@link /OUT:$(VPATH)/$(LIQUIDBIN) /OPT:ICF=4 /INCREMENTAL:NO /NOLOGO /LIBPATH:"$(MAYA_LOCATION)/lib" /LIBPATH:"$(LIQRMANPATH)/lib" /NODEFAULTLIB:libmmt.lib /NODEFAULTLIB:libircmt.lib /IMPLIB:$(VPATH)/liquid$(LIBSUF) /MACHINE:I386 $(LIQWINRMANLIBS) $(LIQUIDBINLIBS) $(VPATH)/liquidBin$(OBJSUF) $(LIQUIDOUTMAINOBJS)

$(LIQUIDMAINOBJS) : ../include/liquid.h
liqRibTranslator$(OBJSUF) : ../include/liqRenderScript.h

.cpp.obj :
	@echo $@
	@$(CPP) -c $(INCLUDES) $(CPPFLAGS) $(OUTFLAG)$(VPATH)/$@ $<

.cpp.i :
	@$(CPP) -E $(INCLUDES) $(CPPFLAGS) $*.cc > $(VPATH)/$*.i

%.iss: force
	@echo [Setup] > ../W32Installer/liquidversion.iss
	@echo AppName=Liquid for ${LIQRMAN} >> ../W32Installer/liquidversion.iss
	@echo AppVersion=${LIQUIDSHORTVERSION} >> ../W32Installer/liquidversion.iss
	@echo AppVerName=Liquid ${VER} for Maya 6 >> ../W32Installer/liquidversion.iss
	@echo OutputBaseFilename=Liquid-${LIQUIDSHORTVERSION}-${LIQRMAN}-Maya-6-Setup >> ../W32Installer/liquidversion.iss
	@echo [Messages] >> ../W32Installer/liquidversion.iss
	@echo BeveledLabel=Liquid for ${LIQRMAN} Setup >> ../W32Installer/liquidversion.iss
	@echo [Files] >> ../W32Installer/liquidversion.iss
	@echo "Source: ..\\bin\\win32\\release\\${LIQRMAN}\\liquid.mll; DestDir: {app}\\bin\\maya6\\${LIQRMAN};" >> ../W32Installer/liquidversion.iss
	@echo "Source: ..\\bin\\win32\\release\\${LIQRMAN}\\liquid.exe; DestDir: {app}\\bin\\maya6\\${LIQRMAN};" >> ../W32Installer/liquidversion.iss
	@echo "Result := '\\${LIQRMAN}';" > ../W32Installer/targetrenderer.iss

force:

$(LIQUIDSETUP) : $(SETUPINCLUDES)
	@echo $@
	@ISCC ..\\W32Installer\\LiquidW32Setup.iss /O..\\W32Installer\\Output /Q

depend :
	makedepend $(INCLUDES) -I/usr/include/CC liquid.cc

clean:
	rm -rf $(VPATH)/*$(PLUGSUF) $(VPATH)/*$(OBJSUF) $(VPATH)/*.d $(VPATH)/ii_files $(VPATH)/so_locations $(SETUPINCLUDES) $(LIQUIDSETUP)
