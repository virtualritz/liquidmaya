DEPTH = .
include $(DEPTH)/commondefs.mk

#MAYA_RELEASES := $(patsubst $(AW_LOCATION)/maya%, %, $(MAYA_VERSIONS) )
MAYA_RELEASES ?= 2009
COMPILED_VERSIONS = $(wildcard bin/redhat/*)
RELEASE_PATH = /net/soft/fscfc/maya
LIQUID_PLUGS = $(wildcard bin/redhat/*/)

ALL_RELEASE_PATHS = $(patsubst bin/$(VBIN)/%,$(RELEASE_PATH)%,$(COMPILED_VERSIONS))

default : debug

clean :
	( cd ribLib && make $@ )
	( cd src && make $@ )
	( cd shaders && make $@ )

ifeq "$(USE_RIBLIB)" "yes"
all debug release :
	( cd ribLib && make )
	( cd src && make BIN_VERSION=$@ $@ )
	( cd shaders && make BIN_VERSION=$@ $@ )
else
all debug release :
	( cd src && make BIN_VERSION=$@ $@ )
	( cd shaders && make BIN_VERSION=$@ $@ )
endif

displaydriverair :
	( cd src && make -f Makefile.dd $@ )

displaydriverpixie :
	( cd src && make -f Makefile.dd $@ )

displaydriveraqsis :
	( cd src && make -f Makefile.dd $@ )

displaydriverdelight :
	( cd src && make -f Makefile.dd $@ )

realclean : 
	rm -rf $(DEPTH)/bin/$(VBIN)
	rm -rf $(DEPTH)/bin/linux32/*

ifeq ($(LIQRMAN),3delight)
install :
	( cd src && make BIN_VERSION="release" install3delight)
else
ifeq ($(LIQRMAN),pixie)
install :
	( cd src && make BIN_VERSION="release" installpixie)
else
ifeq ($(LIQRMAN),air)
install :
	( cd src && make BIN_VERSION="release" installair)
else
install : dist
	@echo "Installing in $(INSTALL_DIR)"
	@mkdir -p $(INSTALL_DIR)
	@for vers in $(strip $(MAYA_RELEASES));\
	do \
		( echo Maya $${vers:=none} ---------------------------------------------; cd src && make MAYA_VERSION=$${vers:=none} BIN_VERSION=$${vers:=none} INSTALL_DIR=$(INSTALL_DIR) installPlug installBin);\
	done;
	@( cd src && make BIN_VERSION=release INSTALL_DIR=$(INSTALL_DIR) installDpy )
	@( cd mel && make VPATHMEL=$(LIQ_MEL_DIR) newversion );
	@( cd icons && make VPATHICONS=$(LIQ_ICONS_DIR) newversion );
	@( cd shaders && make VPATHSHADERS=$(LIQ_SHADERS_DIR) newversion );
	@( cd renderers && make VPATHRENDERS=$(LIQ_RENDERERS_DIR) newversion );

endif
endif
endif


dist :
	@for vers in $(strip $(MAYA_RELEASES));\
	do \
		( echo Maya $${vers:=none} ---------------------------------------------; cd src && make MAYA_VERSION=$${vers:=none} BIN_VERSION=$${vers:=none} newversion );\
	done;
#	@( cd shaders && make newversion );
	@( cd mel && make newversion );
	@( cd icons && make newversion );
	@( cd renderers && make newversion );
