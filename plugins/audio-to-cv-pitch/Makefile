# --------------------------------------------------------------
#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = audio-to-cv-pitch-falk-changes

AUBIO_MODULE = ../../aubio_module
AUBIO = ../../aubio

# --------------------------------------------------------------
# Files to build

FILES_DSP = plugin/plugin.cpp

EXTRA_LIBS = $(AUBIO)/libaubio.a $(shell pkg-config --libs fftw3f)

# --------------------------------------------------------------
# Do some magic

include ../../dpf/Makefile.plugins.mk

BUILD_CXX_FLAGS += \
	-I./plugin \
	-I./dsp \
	-I$(AUBIO_MODULE)/src \
	-I$(AUBIO)/src \

# LINK_FLAGS += $(shell pkg-config --libs fftw3f)

# --------------------------------------------------------------
# Enable all possible plugin types

TARGETS += lv2_dsp

all: $(TARGETS)

# --------------------------------------------------------------
