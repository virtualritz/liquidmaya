SHELL = sh
#BUILDDATE = `date "+,\040%a.,\040%e.\040%b.\040%Y,\040%R"`
LIQUIDSHORTVERSION = `tr -d \"\\"\\"\" < liquid.version`
BUILDDATE = `date "+%d.\040%b.\040%Y"`
LIQUIDVERSION = \"$(LIQUIDSHORTVERSION)\040for\040$(LIQRMAN),\040$(BUILDDATE)\"

#------------------------------------------------------
all : debug

lib newversion clean debug release :
	@if( test `uname` = 'IRIX' || test `uname` = 'IRIX64'); then \
		whichflags=irix65dbg; \
		os=irix; \
	elif( test `uname` = 'Linux' ); then \
		if( test -f "/etc/redhat-release" ); then \
			whichflags=redhat62dbg; \
			os=redhat; \
		elif( test -f "/etc/slackware-version" ); then \
			whichflags=slackware1dbg; \
			os=slackware; \
		elif( test -f "/etc/gentoo-release" ); then \
			whichflags=gentoo14dbg; \
			os=gentoo; \
		else \
			whichflags="suse71dbg"; \
			os=suse; \
		fi; \
	elif( test `uname -o` = 'Cygwin' ); then \
		whichflags=winnt5dbg; \
		os=windows; \
	fi; \
	case $${LIQRMAN} in \
		[aA]qsis) LIQRMANPATH=$${AQSIS_BASE_PATH:=/usr/local/aqsis} LIQRMANFLAGS=-DAQSIS LIQRMANLIBS="-lshaderexecenv -lshadervm -laqsistypes -lri2rib -lslxargs" LIQWINRMANLIBS="ri2rib.lib slxargs.lib" USEDVAR=AQSIS_BASE_PATH;; \
		[eE]ntropy) LIQRMANPATH=$${ENTROPYHOME:=/usr/local/exluna/Entropy} LIQRMANFLAGS=-DENTROPY LIQRMANLIBS="-lribout -lsleargs" USEDVAR=ENTROPYHOME;; \
		[pP][rR][mM]an) LIQRMANPATH=$${RMANTREE:=/usr/local/prman} LIQRMANFLAGS=-DPRMAN LIQRMANLIBS="-lrib -lsloargs -llkm -ltarget -lzip" LIQWINRMANLIBS="rib.lib sloargs.lib" USEDVAR=RMANTREE;; \
		[pP][rR][mM]an12_5) LIQRMANPATH=$${RMANTREE:=/usr/local/prman} LIQRMANFLAGS=-DPRMAN LIQRMANLIBS="-lrib -lsloargs -llkm -ltarget -lzip -lprmutil" LIQWINRMANLIBS="rib.lib sloargs.lib" USEDVAR=RMANTREE;; \
		[pP]ixie) LIQRMANPATH=$${PIXIEHOME:=/usr/local/Pixie} LIQRMANFLAGS=-DPIXIE LIQRMANLIBS="-lri -lsdr" LIQWINRMANLIBS="ri.lib sdr.lib" USEDVAR=PIXIEHOME;; \
		3[dD]elight) LIQRMANPATH=$${DELIGHT:=/usr/local/3delight} LIQRMANFLAGS=-DDELIGHT LIQRMANLIBS="-l3delight -lc -ldl -lm" LIQWINRMANLIBS="3delight.lib" USEDVAR=DELIGHT;; \
		*) echo "Do not know how to handle" $${LIQRMAN}". Set LIQRMAN to either aqsis, entropy, prman, prman12_5, pixie or 3delight"; exit 1;; \
	esac; \
	if( test ! -d  $${LIQRMANPATH} ); then \
		echo $${LIQRMANPATH} " does not seem to exist. Check your" $${USEDVAR} "env var."; exit 1; \
	fi; \
	if( test $@ = 'debug' ); then \
		EXTRAFLAGS="-g -DDEBUG -DLIQUIDVERSION=\\\"$(LIQUIDVERSION)\\\"" ; \
		MAYA_LOCATION=$${MAYA_LOCATION:=/usr/aw/maya} ;\
		if( test -d $${MAYA_LOCATION}); then\
			( echo Got $${whichflags:=toto} $${LIQRMAN} $${LIQRMANPATH}; LIQUIDSHORTVERSION=$(LIQUIDSHORTVERSION) LIQUIDVERSION=$(LIQUIDVERSION) LIQRMANPATH=$${LIQRMANPATH} LIQRMANFLAGS=$${LIQRMANFLAGS} LIQRMANLIBS=$${LIQRMANLIBS} LIQWINRMANLIBS=$${LIQWINRMANLIBS} EXTRAFLAGS=$${EXTRAFLAGS} MAYA_LOCATION=$${MAYA_LOCATION} BIN_VERSION="debug" $${make:=make} -f Makeopts.$${os} $@ ); \
		else \
			echo $${MAYA_LOCATION} " does not seem to exists. Please set MAYA_LOCATION to a correct value."; exit 1; \
		fi; \
	elif( test $@ = 'release' ); then \
		EXTRAFLAGS="-DLIQUIDVERSION=\\\"$(LIQUIDVERSION)\\\"" ; \
		MAYA_LOCATION=$${MAYA_LOCATION:=/usr/aw/maya} ;\
		if( test -d $${MAYA_LOCATION}); then\
			( echo Got $${whichflags:=toto} $${LIQRMAN} $${LIQRMANPATH}; LIQUIDSHORTVERSION=$(LIQUIDSHORTVERSION) LIQUIDVERSION=$(LIQUIDVERSION) LIQRMANPATH=$${LIQRMANPATH} LIQRMANFLAGS=$${LIQRMANFLAGS} LIQRMANLIBS=$${LIQRMANLIBS} LIQWINRMANLIBS=$${LIQWINRMANLIBS} EXTRAFLAGS=$${EXTRAFLAGS} MAYA_LOCATION=$${MAYA_LOCATION} BIN_VERSION="release" $${make:=make} -f Makeopts.$${os} $@ ); \
		else \
			echo $${MAYA_LOCATION} " does not seem to exists. Please set MAYA_LOCATION to a correct value."; exit 1; \
		fi; \
	else \
		AW_LOCATION=$${AW_LOCATION:=/usr/aw} ;\
		EXTRAFLAGS="-O -DLIQUIDVERSION=\\\"$(LIQUIDVERSION)\\\"" ; \
		( echo Got for newversion $${whichflags:=toto} $${LIQRMAN} $${LIQRMANPATH}; LIQUIDSHORTVERSION=$(LIQUIDSHORTVERSION) LIQUIDVERSION=$(LIQUIDVERSION) LIQRMANPATH=$${LIQRMANPATH} LIQRMANFLAGS=$${LIQRMANFLAGS} LIQRMANLIBS=$${LIQRMANLIBS} LIQWINRMANLIBS=$${LIQWINRMANLIBS} EXTRAFLAGS=$${EXTRAFLAGS} MAYA_LOCATION=$${MAYA_LOCATION} BIN_VERSION="debug" $${make:=make} -f Makeopts.$${os} $@ ); \
		( echo Got for newversion $${whichflags:=toto} $${LIQRMAN} $${LIQRMANPATH}; LIQUIDSHORTVERSION=$(LIQUIDSHORTVERSION) LIQUIDVERSION=$(LIQUIDVERSION) LIQRMANPATH=$${LIQRMANPATH} LIQRMANFLAGS=$${LIQRMANFLAGS} LIQRMANLIBS=$${LIQRMANLIBS} LIQWINRMANLIBS=$${LIQWINRMANLIBS} EXTRAFLAGS=$${EXTRAFLAGS} MAYA_LOCATION=$${MAYA_LOCATION} BIN_VERSION="release" $${make:=make} -f Makeopts.$${os} $@ ); \
		if( test -d $${AW_LOCATION}); then\
			for vers in $${AW_LOCATION}/maya*.*;\
				do ( echo Got $${whichflags} $${vers:=none} $${vers#$${AW_LOCATION}/}; LIQUIDVERSION=$(LIQUIDVERSION) LIQRMANPATH=$${LIQRMANPATH} LIQRMANFLAGS=$${LIQRMANFLAGS} LIQRMANLIBS=$${LIQRMANLIBS} LIQWINRMANLIBS=$${LIQWINRMANLIBS} EXTRAFLAGS=$${EXTRAFLAGS} MAYA_LOCATION=$${vers:=none} BIN_VERSION=$${vers#$${AW_LOCATION}/} $${make:=make} -f Makeopts.$${os} $@ );\
			done; \
		else \
			echo $${AW_LOCATION} " does not seem to exists. Please set AW_LOCATION to a correct value."; exit 1; \
		fi; \
	fi
# DO NOT DELETE
