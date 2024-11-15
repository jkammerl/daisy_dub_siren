# Project Name
TARGET = DubSiren

# Sources
CPP_SOURCES = main.cpp sample_info_storage.cpp sample_search.cpp sdcard.cpp wav_file.cpp sample_player.cpp sample_manager.cpp adc.cpp switches.cpp sample_scanner.cpp



USE_DAISYSP_LGPL = 1

USE_FATFS = 1
_USE_FIND = 1
#_FS_TINY = 1
_USE_FASTSEEK = 1
_VOLUMES=1
_USE_STRFUNC=0

DEBUG=1

C_INCLUDES += \
-I. \
-Iresources \
-I../..

#LDFLAGS = -u _printf_float
APP_TYPE = BOOT_SRAM

# Library Locations
LIBDAISY_DIR ?= ../../libDaisy/
DAISYSP_DIR ?= ../../DaisySP/
STMLIB_DIR ?= ../../stmlib

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
