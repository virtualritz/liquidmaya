DEPTH = .
include $(DEPTH)/commondefs.mk

MAYA_RELEASES := $(patsubst $(AW_LOCATION)/maya%, %, $(MAYA_VERSIONS) )
MAYA_RELEASES := 7.0.1
COMPILED_VERSIONS = $(wildcard bin/redhat/*)
RELEASE_PATH = /net/soft/fscfc/maya
LIQUID_PLUGS = $(wildcard bin/redhat/*/)

ALL_RELEASE_PATHS = $(patsubst bin/$(VBIN)/%,$(RELEASE_PATH)%,$(COMPILED_VERSIONS))

default : debug

all clean debug release :
	( cd src && make $@)
	( cd shaders && make $@)

realclean : 
	rm -rf $(DEPTH)/bin/$(VBIN)

dist :
	@for vers in $(strip $(MAYA_RELEASES));\
	do \
		( echo Maya $${vers:=none} ---------------------------------------------; cd src && MAYA_VERSION=$${vers:=none} BIN_VERSION=$${vers:=none} make newversion );\
	done;
	@( cd shaders && make newversion );
	@( cd mel && make newversion );
	@( cd icons && make newversion );
	@( cd renderers && make newversion );
