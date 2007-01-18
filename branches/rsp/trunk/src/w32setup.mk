DEPTH=..
include ../commondefs.mk

SETUPPATH	= $(DEPTH)/W32Installer
MAYA_VERSION=7
LIQUIDSETUP = $(SETUPPATH)/Output/Liquid-$(LIQUIDSHORTVERSION)-$(LIQRMAN)-Maya-$(MAYA_VERSION)-Setup.exe

SETUPINCLUDES = \
	$(SETUPPATH)/liquidversion.iss \
	$(SETUPPATH)/targetrenderer.iss

all: $(LIQUIDSETUP)


%.iss:
	@echo [Setup] > ../W32Installer/liquidversion.iss
	@echo AppName=Liquid for ${LIQRMAN} >> ../W32Installer/liquidversion.iss
	@echo AppVersion=${LIQUIDSHORTVERSION} >> ../W32Installer/liquidversion.iss
	@echo AppVerName=Liquid ${VER} for Maya ${MAYA_VERSION} >> ../W32Installer/liquidversion.iss
	@echo OutputBaseFilename=Liquid-${LIQUIDSHORTVERSION}-${LIQRMAN}-Maya-${MAYA_VERSION}-Setup >> ../W32Installer/liquidversion.iss
	@echo [Messages] >> ../W32Installer/liquidversion.iss
	@echo BeveledLabel=Liquid for ${LIQRMAN} Setup >> ../W32Installer/liquidversion.iss
	@echo [Files] >> ../W32Installer/liquidversion.iss
	@echo "Source: ..\\bin\\${LIQRMAN}\\win32\\release\\liquid.mll; DestDir: {app}\\bin\\maya${MAYA_VERSION}\\${LIQRMAN};" >> ../W32Installer/liquidversion.iss
	@echo "Source: ..\\bin\\${LIQRMAN}\\win32\\release\\liquid.exe; DestDir: {app}\\bin\\maya${MAYA_VERSION}\\${LIQRMAN};" >> ../W32Installer/liquidversion.iss
	@echo "Result := '\\${LIQRMAN}';" > ../W32Installer/targetrenderer.iss

$(LIQUIDSETUP): $(SETUPINCLUDES) ../W32Installer/LiquidW32Setup.iss
	@echo $@
	@ISCC ..\\W32Installer\\LiquidW32Setup.iss /O..\\W32Installer\\Output /Q
