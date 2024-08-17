# Project Name
TARGET = DubSiren

# Sources
CPP_SOURCES = main.cpp sdcard.cpp wav_file.cpp sample_player.cpp sample_manager.cpp adc.cpp switches.cpp


USE_DAISYSP_LGPL = 1

USE_FATFS = 1
_USE_FIND = 1

DEBUG=1

C_INCLUDES += \
-I. \
-Iresources \
-I../..

#LDFLAGS = -u _printf_float

# Library Locations
LIBDAISY_DIR ?= ../../libDaisy/
DAISYSP_DIR ?= ../../DaisySP/
STMLIB_DIR ?= ../../stmlib

C_SOURCES = $(wildcard ../../libDaisy/Drivers/CMSIS/DSP/Source/*/*.c)

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
