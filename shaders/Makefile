# Common definition to compile shaders
DEPTH = ..
include $(DEPTH)/commondefs.mk

INCLUDES 	= -I./include -I$(LIQRMANPATH)/include

SLSHADERS	= $(wildcard src/*.sl)
SLOSHADERS	= $(SLSHADERS:.sl=.slo)

SRCS			= $(SLSHADERS)
VPATHDEBUG		= $(DEPTH)/bin/$(VBIN)/debug/$(LIQRMAN)/shaders
VPATHRELEASE		= $(DEPTH)/bin/$(VBIN)/release/$(LIQRMAN)/shaders

DEBUGS 			= $(patsubst src/%,$(VPATHDEBUG)/%, $(SLOSHADERS))
RELEASES 		= $(patsubst src/%,$(VPATHRELEASE)/%, $(SLOSHADERS))
TARGETS 		= $(patsubst src/%,$(VPATHSHADERS)/%, $(SLOSHADERS))



#BIN_VERSION=debug

.SUFFIXES :
.SUFFIXES : .slo .sl

all : debug

.sl.slo:
	$(CSL) $(SLFLAGS) $(INCLUDES) -o $@ $<

debug : $(VPATHDEBUG) $(DEBUGS)
release : $(VPATHRELEASE) $(RELEASES)
newversion : $(VPATHSHADERS) $(TARGETS)

$(VPATHDEBUG) $(VPATHSHADERS) $(VPATHRELEASE) :
	mkdir -p $@

$(VPATHSHADERS)/%.slo : src/%.sl
	@echo $@
	@$(CSL) $(SLFLAGS) $(INCLUDES) -o $@ $<

$(VPATHDEBUG)/%.slo : src/%.sl
	@echo $@
	@$(CSL) $(SLFLAGS) $(INCLUDES) -o $@ $<

$(VPATHRELEASE)/%.slo : src/%.sl
	@echo $@
	@$(CSL) $(SLFLAGS) $(INCLUDES) -o $@ $<


clean Clean :
	@rm -f $(SLOSHADERS) *.slo

include $(DEPTH)/commonrules.mk
