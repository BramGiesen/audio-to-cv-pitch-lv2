#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX, Christopher Arndt, and Patrick Desaulniers
#

include dpf/Makefile.base.mk

all: libs plugins gen

# --------------------------------------------------------------

ifneq ($(CROSS_COMPILING),true)
CAN_GENERATE_TTL = true
else ifneq ($(EXE_WRAPPER),)
CAN_GENERATE_TTL = true
endif

libs:
	$(MAKE) -C aubio

plugins: libs
	$(MAKE) all -C plugins/audio-to-cv-pitch

ifeq ($(CAN_GENERATE_TTL),true)
gen: plugins dpf/utils/lv2_ttl_generator
	@$(CURDIR)/dpf/utils/generate-ttl.sh
# 	cp -r static-lv2-data/audio-to-cv-pitch.lv2/* bin/audio-to-cv-pitch.lv2/

dpf/utils/lv2_ttl_generator:
	$(MAKE) -C dpf/utils/lv2-ttl-generator
else
gen:
endif

# --------------------------------------------------------------

clean:
	$(MAKE) clean -C dpf/utils/lv2-ttl-generator
	$(MAKE) clean -C plugins/audio-to-cv-pitch
	$(MAKE) clean -C aubio
	rm -rf bin build
	# missed by dpf, clean it ourselves
	rm -f aubio_module/src/*.d
	rm -f aubio_module/src/*.o

# --------------------------------------------------------------

.PHONY: all clean install install-user plugins submodule
