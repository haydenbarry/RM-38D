# Project Name
TARGET = RM-38D

# Sources
CPP_SOURCES = RM-38D.cpp
CPP_SOURCES += EncoderPush.cpp
CPP_SOURCES += InputDriver.cpp
CPP_SOURCES += SampleVoice.cpp
CPP_SOURCES += Sequence.cpp
CPP_SOURCES += StepButtons.cpp
CPP_SOURCES += StepLEDs.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR = ../../DaisySP/
OPT = -Os
USE_FATFS = 1
# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
