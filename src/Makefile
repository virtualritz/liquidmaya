SHELL = sh
LIQUIDVERSION 	= "1.5.3"
#------------------------------------------------------
all : debug

lib newversion clean debug release : 
	@if( test `uname` = 'IRIX' || test `uname` = 'IRIX64'); then\
	    whichflags=irix65dbg;\
	    os=irix;\
	elif( test `uname` = 'Linux' ); then\
	    if( test -f "/etc/redhat-release" ); then\
		whichflags=redhat62dbg;\
		os=redhat;\
	    else \
		whichflags="suse71dbg";\
		os=suse;\
	    fi;\
	fi; \
	LIQRMAN=$${LIQRMAN:=aqsis}; \
	case $${LIQRMAN} in \
	    aqsis) LIQRMANPATH=$${AQSIS_BASE_PATH:=/usr/local/aqsis} LIQRMANFLAGS=-DAQSIS LIQRMANLIBS="-lshaderexecenv -lshadervm -laqsistypes -lri2rib -lslxargs" USEDVAR=AQSIS_BASE_PATH;; \
	    entropy) LIQRMANPATH=$${ENTROPYHOME:=/usr/local/exluna/Entropy} LIQRMANFLAGS=-DENTROPY LIQRMANLIBS="-lribout -lsleargs" USEDVAR=ENTROPYHOME;; \
	    prman) LIQRMANPATH=$${RMANTREE:=/usr/local/prman} LIQRMANFLAGS=-DPRMAN LIQRMANLIBS="-lrib -lsloargs -llkm -ltarget -lzip" USEDVAR=RMANTREE;; \
	    prman11) LIQRMANPATH=$${RMANTREE:=/usr/local/prman} LIQRMANFLAGS=-DPRMAN LIQRMANLIBS="-lrib -lsloargs -llkm -ltarget -lzip -lprmutil" USEDVAR=RMANTREE;; \
	    (*) echo "Do not know how to handle" $${LIQRMAN}". Set LIQRMAN to either aqsis entropy or prman"; exit 1;; \
	esac; \
	if( test ! -d  $${LIQRMANPATH} ); then \
	    echo $${LIQRMANPATH} "do not seem to exist. Check your" $${USEDVAR} "env var."; exit 1; \
	fi; \
	if( test $@ = 'debug' ); then \
	    EXTRAFLAGS="-g -DDEBUG -DLIQUIDVERSION=\\\"$(LIQUIDVERSION)\\\""; \
	    MAYA_LOCATION=$${MAYA_LOCATION:=/usr/aw/maya} ;\
	    if( test -d $${MAYA_LOCATION}); then\
	    	( echo Got $${whichflags:=toto} $${LIQRMAN} $${LIQRMANPATH}; LIQUIDVERSION=$(LIQUIDVERSION) LIQRMANPATH=$${LIQRMANPATH} LIQRMANFLAGS=$${LIQRMANFLAGS} LIQRMANLIBS=$${LIQRMANLIBS} EXTRAFLAGS=$${EXTRAFLAGS} MAYA_LOCATION=/usr/aw/maya MAYA_VERSION="debug" $${make:=make} -f Makefile.$${os} $@ ); \
	    else \
		echo $${MAYA_LOCATION} " do not seem to exists. Please set MAYA_LOCATION to a correct value."; exit 1; \
	    fi; \
	elif( test $@ = 'release' ); then \
	    EXTRAFLAGS="-O -DLIQUIDVERSION=\\\"$(LIQUIDVERSION)\\\""; \
	    MAYA_LOCATION=$${MAYA_LOCATION:=/usr/aw/maya} ;\
	    if( test -d $${MAYA_LOCATION}); then\
	    	( echo Got $${whichflags:=toto} $${LIQRMAN} $${LIQRMANPATH}; LIQUIDVERSION=$(LIQUIDVERSION) LIQRMANPATH=$${LIQRMANPATH} LIQRMANFLAGS=$${LIQRMANFLAGS} LIQRMANLIBS=$${LIQRMANLIBS} EXTRAFLAGS=$${EXTRAFLAGS} MAYA_LOCATION=/usr/aw/maya MAYA_VERSION="release" $${make:=make} -f Makefile.$${os} $@ ); \
	    else \
		echo $${MAYA_LOCATION} " do not seem to exists. Please set MAYA_LOCATION to a correct value."; exit 1; \
	    fi; \
	else \
	    AW_LOCATION=$${AW_LOCATION:=/usr/aw} ;\
	    EXTRAFLAGS="-O -DLIQUIDVERSION=\\\"$(LIQUIDVERSION)\\\""; \
	    ( echo Got for newversion $${whichflags:=toto} $${LIQRMAN} $${LIQRMANPATH}; LIQUIDVERSION=$(LIQUIDVERSION) LIQRMANPATH=$${LIQRMANPATH} LIQRMANFLAGS=$${LIQRMANFLAGS} LIQRMANLIBS=$${LIQRMANLIBS} EXTRAFLAGS=$${EXTRAFLAGS} MAYA_LOCATION=/usr/aw/maya MAYA_VERSION="debug" $${make:=make} -f Makefile.$${os} $@ ); \
	    if( test -d $${AW_LOCATION}); then\
		for vers in $${AW_LOCATION}/maya*.*;\
			do ( echo Got $${whichflags} $${vers:=none} $${vers#$${AW_LOCATION}/}; LIQUIDVERSION=$(LIQUIDVERSION) LIQRMANPATH=$${LIQRMANPATH} LIQRMANFLAGS=$${LIQRMANFLAGS} LIQRMANLIBS=$${LIQRMANLIBS} EXTRAFLAGS=$${EXTRAFLAGS} MAYA_LOCATION=$${vers:=none} MAYA_VERSION=$${vers#$${AW_LOCATION}/} $${make:=make} -f Makefile.$${os} $@ );\
		done; \
	    else \
		echo $${AW_LOCATION} " do not seem to exists. Please set AW_LOCATION to a correct value."; exit 1; \
	    fi; \
	fi;
