# Project Name
TARGET = DubSiren

# Sources
CPP_SOURCES = main.cpp adc.cpp switches.cpp

USE_DAISYSP_LGPL = 1

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

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
