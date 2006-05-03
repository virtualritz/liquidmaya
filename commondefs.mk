SHELL = sh


# Set default if variables not set

AW_LOCATION ?= /usr/aw
MAYA_LOCATION ?= $(AW_LOCATION)/maya
MAYA_VERSIONS := $(wildcard $(AW_LOCATION)/maya*)
MAYA_VERSION ?= 7.0
LIQRMAN ?= pixie
LIQ_OLD_MAYA_IDS ?= 0
# Os settings
# What about Suze ?

OS_NAME := $(shell uname -o)

ifeq "$(OS_NAME)" "GNU/Linux"
PLUGSUF       = so
OBJEXT        = o
LD            = $(MAYA_LOCATION)/bin/mayald
LOCFLAGS      = -ffast-math -march=pentiumpro -D_BOOL -DLINUX -DLIQ_OLD_MAYA_IDS=$(LIQ_OLD_MAYA_IDS)
WARNFLAGS     = -Wall -W -Wno-comment -Wpointer-arith -Wno-inline -Woverloaded-virtual -Wno-sign-compare -Wno-parentheses -Wno-deprecated
NO_TRANS_LINK =
VBIN          = redhat
EXTRA_LIBS    =
AR            = ar cqs
CPP 			= gcc

UX_RELEASE_TEST_FILES = /etc/fedora-release /etc/redhat-release /etc/slackware-version /etc/gentoo-release
UX_RELEASE_FILE ?= $(foreach FILE, $(UX_RELEASE_TEST_FILES), $(wildcard $(FILE)*))
FLAVORX := $(patsubst /etc/%-release,%,$(UX_RELEASE_FILE))
FLAVORX := $(patsubst /etc/%-version,%,$(FLAVORX))

# Anything special for some linux flavor ?
ifeq "$(FLAVORX)" "fedora redhat"
VBIN	= fedora
# Make sure we use gcc 3.3.4 if 3.4.4 is default one
ifeq "$(shell $(CPP) -dumpversion)" "3.4.4"
CPP = g++334
endif
else
VBIN	= $(FLAVORX)
endif

else
ifeq "$(OS_NAME)" "Cygwin"
# Cygwin 
FLAVORX = cygwin
PLUGSUF			= .mll
BINSUF			= .exe
OBJSUF			= .obj
LIBSUF			= .lib
CPP				= cl
LINK			= link
LD				= $(MAYA_LOCATION)/bin/mayald
LOCFLAGS		= -nologo -O2 -EHsc -MT -YXstdafx.h -FD -GR -D_BOOL -D_WIN32 -DWIN32 -D_USRDLL -DLIQUID_EXPORTS -Dulong="unsigned long" -Duint="unsigned int"
WARNFLAGS		= -W1
VBIN			= win32
AR				= ar cqs
LINKEROPTS		= -INCREMENTAL:NO -NOLOGO -NODEFAULTLIB:libircmt$(LIBEXT) -PDB:liquid.pdb -MACHINE:I386
LIBPATHFLAG		= -LIBPATH:
LIBFLAG			=
SHAREDFLAG		= -DLL
OUTFLAG			= -Fo
EXTRALIBS		= $(LIBFLAG)shell32$(LIBSUF) $(LIBFLAG)Foundation$(LIBSUF)

endif

# Not Linux, not Cygwin ...what is it ?
FLAVORX = "notunix"
endif

LIQUIDSHORTVERSION := $(shell tr -d \"\\"\\"\" < $(DEPTH)/src/liquid.version)
BUILDDATE := $(shell date "+%d.\040%b.\040%Y")
LIQUIDVERSION = \"$(LIQUIDSHORTVERSION)\040for\040$(LIQRMAN),\040$(BUILDDATE)\"


# Renderers setting
# Pixie
ifeq "$(LIQRMAN)" "pixie"
LIQDISPLAYOBJS  = 	liqMayaDisplayDriverPixie.$(OBJEXT)
LIQRMANPATH 	= 	$${PIXIEHOME:=/usr/local/Pixie}
LIQRMANFLAGS	=	-DPIXIE
LIQRMANLIBS 	=	-lri -lsdr
LIQWINRMANLIBS	=	"ri.lib sdr.lib" 
USEDVAR 		=	PIXIEHOME
CSL 			= 	$(LIQRMANPATH)/bin/sdrc
SLFLAGS 		= 	-DPIXIE
endif

# Aqsis
ifeq "$(LIQRMAN)" "aqsis"
LIQDISPLAYOBJS  = 	liqMayaDisplayDriverAqsis.$(OBJEXT)
LIQRMANPATH 	=	$${AQSIS_BASE_PATH:=/usr/local/aqsis}
LIQRMANFLAGS	=	-DAQSIS
LIQRMANLIBS 	=	-lshaderexecenv -lshadervm -laqsistypes -lri2rib -lslxargs
LIQWINRMANLIBS	=	ri2rib.lib slxargs.lib 
USEDVAR 		=	AQSIS_BASE_PATH
CSL 			= 	$(LIQRMANPATH)/bin/aqsl
SLFLAGS 		= 	-DAQSIS
endif

# 3delight
ifeq "$(LIQRMAN)" "3delight"
LIQDISPLAYOBJS  = liqMayaDisplayDriver3delight.$(OBJEXT)
LIQRMANPATH 	=	$${DELIGHT:=/usr/local/3delight}
LIQRMANFLAGS	=	-DDELIGHT
LIQRMANLIBS 	=	-l3delight -lc -ldl -lm
LIQWINRMANLIBS	=	3delight.lib
USEDVAR 		=	DELIGHT
CSL 			= 	$(LIQRMANPATH)/bin/shaderl
SLFLAGS 		=	-DDELIGHT
endif

# Entropy
ifeq "$(LIQRMAN)" "entropy"
LIQDISPLAYOBJS  = 	liqMayaDisplayDriverEntropy.$(OBJEXT)
LIQRMANPATH 	=	$${ENTROPYHOME:=/usr/local/exluna/Entropy}
LIQRMANFLAGS	=	-DENTROPY
LIQRMANLIBS 	=	-lribout -lsleargs 
USEDVAR 		=	ENTROPYHOME
CSL 			= 	$(LIQRMANPATH)/bin/sle
SLFLAGS 		= 	-DENTROPY
endif

# Prman
ifeq "$(patsubst prman%,prman,$(LIQRMAN))" "prman"
PRMANVERSION 	= 	$(patsubst prman%,%,$(LIQRMAN))
LIQRMANPATH 	=	$${RMANTREE:=/usr/local/prman}
LIQRMANFLAGS	=	-DPRMAN
USEDVAR 		= 	RMANTREE
CSL 			= 	$(LIQRMANPATH)/bin/shader
SLFLAGS 		= 	-DPRMAN
LIQDISPLAYOBJS  = 	liqMayaDisplayDriver.$(OBJEXT)

ifeq "$(patsubst 12.%,12,$(PRMANVERSION))" "12"
LIQRMANLIBS 	=	-lrib -lsloargs -llkm -ltarget -lzip -lprmutil
LIQWINRMANLIBS	= "rib.lib sloargs.lib"
else
ifeq "$(patsubst 13.%,13,$(PRMANVERSION))" "13"
LIQRMANLIBS 	=	-lprmansdk
LIQWINRMANLIBS	=	prmansdk.lib
else
LIQRMANLIBS=-lrib -lsloargs -llkm -ltarget -lzip
LIQWINRMANLIBS	=	rib.lib sloargs.lib
endif

endif

endif



INSTALL_DIR  = /temp/maya$(MAYA_VERSION)/liquid
#INSTALL_DIR  = /net/soft/fscfc/maya$(MAYA_VERSION)/liquid
INSTALL_PLUG_DIR = /temp/maya$(MAYA_VERSION)/liquid

LIQ_ICONS_DIR   	= $(INSTALL_DIR)/icons
LIQ_SHADERS_SRC_DIR = $(INSTALL_DIR)/shaders/src
LIQ_SHADERS_DIR 	= $(INSTALL_DIR)/shaders
LIQ_RENDERERS_DIR 	= $(INSTALL_DIR)/renderers

BIN_VERSION = $(LIQUIDSHORTVERSION)
#VPATH		= $(DEPTH)/bin/$(VBIN)/$(BIN_VERSION)/$(LIQRMAN)/$(MAYA_VERSION)
VPATHDEBUG	= $(DEPTH)/bin/$(VBIN)/debug/$(LIQRMAN)/$(MAYA_VERSION)
VPATHRELEASE	= $(DEPTH)/bin/$(VBIN)/$(BIN_VERSION)/$(LIQRMAN)/$(MAYA_VERSION)
VPATHSHADERS = $(DEPTH)/bin/$(VBIN)/$(BIN_VERSION)/$(LIQRMAN)/shaders
VPATHMEL = $(DEPTH)/bin/$(VBIN)/$(BIN_VERSION)/$(LIQRMAN)/mel
VPATHRENDERS = $(DEPTH)/bin/$(VBIN)/$(BIN_VERSION)/$(LIQRMAN)/renderers
VPATHICONS = $(DEPTH)/bin/$(VBIN)/$(BIN_VERSION)/$(LIQRMAN)/icons
CPPFLAGS	= -DLIQUIDVERSION=$(LIQUIDVERSION) $(LOCFLAGS) $(LIQRMANFLAGS) $(WARNFLAGS) $(EXTRAFLAGS) $(NO_TRANS_LINK) -DREQUIRE_IOSTREAM
INCLUDES	= -I. -I.. -I$(MAYA_LOCATION)/include -I$(LIQRMANPATH)/include -I../include
LDFLAGS		= $(CPPFLAGS) -L$(MAYA_LOCATION)/lib -L$(LIQRMANPATH)/lib -L/usr/lib
MAYALIBS	= -lOpenMaya -lOpenMayaRender -lOpenMayaUI -lOpenMayaAnim -lOpenMayaFX -lxpcom -lGL -lGLU
LIBS		= $(LIQRMANLIBS) $(MAYALIBS) $(EXTRA_LIBS) -lm
TARGET  	= $(VPATH)/$(LIQUIDPLUG)

CP 		= cp
