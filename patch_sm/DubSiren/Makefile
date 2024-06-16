# Project Name
TARGET = DubSiren

# Sources
CPP_SOURCES = main.cpp sdcard.cpp wav_file.cpp sample_player.cpp sample_manager.cpp adc.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR = ../../DaisySP/

USE_FATFS = 1
_USE_FIND = 1

DEBUG=1

LDFLAGS = -u _printf_float

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
