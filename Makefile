#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX, Christopher Arndt, and Patrick Desaulniers
#

include dpf/Makefile.base.mk

all: libs plugins gen

# --------------------------------------------------------------

libs:
	$(MAKE) -C aubio
	$(MAKE) -C aubio_module

plugins: libs
	$(MAKE) all -C plugins/audio-to-cv-pitch

ifneq ($(CROSS_COMPILING),true)
gen: plugins dpf/utils/lv2_ttl_generator
	#@$(CURDIR)/dpf/utils/generate-ttl.sh
	cp -r static-lv2-data/audio-to-cv-pitch.lv2/* bin/audio-to-cv-pitch.lv2/
ifeq ($(MACOS),true)
	@$(CURDIR)/dpf/utils/generate-vst-bundles.sh
endif
dpf/utils/lv2_ttl_generator:
	$(MAKE) -C dpf/utils/lv2-ttl-generator
else
gen: plugins dpf/utils/lv2_ttl_generator.exe
	#$@(CURDIR)/dpf/utils/generate-ttl.sh
	cp -r static-lv2-data/audio-to-cv-pitch.lv2/* bin/audio-to-cv-pitch.lv2/

dpf/utils/lv2_ttl_generator.exe:
	$(MAKE) -C dpf/utils/lv2-ttl-generator WINDOWS=true
endif

# --------------------------------------------------------------

clean:
	$(MAKE) clean -C dpf/utils/lv2-ttl-generator
	$(MAKE) clean -C plugins/audio-to-cv-pitch
	$(MAKE) clean -C aubio_module
	rm -rf bin build
# --------------------------------------------------------------

.PHONY: all clean install install-user plugins submodule
