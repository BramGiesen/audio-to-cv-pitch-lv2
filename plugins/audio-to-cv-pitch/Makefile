# --------------------------------------------------------------
#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = audio-to-cv-pitch

AUBIO_MODULE = ../../aubio_module
AUBIO = ../../aubio

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	plugin/plugin.cpp \
	$(AUBIO_MODULE)/src/hopbuffer.cpp \
	$(AUBIO_MODULE)/src/aubio_module.cpp \
	$(AUBIO_MODULE)/src/aubio_pitch.cpp

EXTRA_LIBS = $(AUBIO)/libaubio.a

# --------------------------------------------------------------
# Do some magic

include ../../dpf/Makefile.plugins.mk

BUILD_CXX_FLAGS += \
	-I./plugin \
	-I./dsp \
	-I$(AUBIO_MODULE)/src \
	-I$(AUBIO)/src \

# --------------------------------------------------------------
# Enable all possible plugin types

TARGETS += lv2_dsp

all: $(TARGETS)

# --------------------------------------------------------------
