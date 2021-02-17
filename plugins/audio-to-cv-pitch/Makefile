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

# --------------------------------------------------------------
# Files to build

FILES_DSP  = \
			plugin/plugin.cpp \
			$(AUBIO_MODULE)/src/hopbuffer.cpp \
			$(AUBIO_MODULE)/src/aubio_module.cpp \
			$(AUBIO_MODULE)/src/aubio_pitch.cpp

# --------------------------------------------------------------
# Do some magic

include ../../dpf/Makefile.plugins.mk


BUILD_CXX_FLAGS += \
				   -I./plugin \
				   -I./dsp \
				   -I$(AUBIO_MODULE)/src \

ifeq ($(WIN32),true)
LINK_OPTS += -static -static-libgcc -static-libstdc++ aubio_module/aubio/aubio.a
endif

BASE_FLAGS += -I$(AUBIO_MODULE)/aubio/src
LINK_FLAGS += $(AUBIO_MODULE)/aubio/build/src/libaubio.a


# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(HAVE_DGL),true)
TARGETS += lv2_sep
else
TARGETS += lv2_dsp
endif

all: $(TARGETS)

# --------------------------------------------------------------
