######################################################################
# makefile
#
# Builds the RTK support programs
######################################################################

##########
# Source files
##########

EXECUTABLES  = Compare
EXECUTABLES += NMEA_Client
EXECUTABLES += Read_Map_File
EXECUTABLES += RTK_Reset
EXECUTABLES += Split_Messages
EXECUTABLES += X.509_crt_bundle_bin_to_c

INCLUDES  = crc24q.h

##########
# Buid tools and rules
##########

GCC = gcc
CFLAGS = -flto -O3 -Wpedantic -pedantic-errors -Wall -Wextra -Werror -Wno-unused-variable -Wno-unused-parameter
CC = $(GCC) $(CFLAGS)

%.o: %.c $(INCLUDES)
	$(CC) -c -o $@ $<

%: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ $<

##########
# Buid all the sources - must be first
##########

.PHONY: all

all: $(EXECUTABLES)

##########
# Buid RTK firmware
##########

DEBUG_LEVEL=debug
ENABLE_DEVELOPER=false
FIRMWARE_VERSION_MAJOR=99
FIRMWARE_VERSION_MINOR=99
POINTPERFECT_TOKEN=

RTK:	../RTK_Surveyor/RTK_Surveyor.ino ../RTK_Surveyor/*.h
	~/Arduino/arduino-cli compile --fqbn "esp32:esp32:esp32":DebugLevel=$(DEBUG_LEVEL) ../RTK_Surveyor/RTK_Surveyor.ino  --build-property build.partitions=app3M_fat9M_16MB  --build-property upload.maximum_size=3145728 --build-property "compiler.cpp.extra_flags=\"-DPOINTPERFECT_TOKEN=$(POINTPERFECT_TOKEN)\" \"-DFIRMWARE_VERSION_MAJOR=$(FIRMWARE_VERSION_MAJOR)\" \"-DFIRMWARE_VERSION_MINOR=$(FIRMWARE_VERSION_MINOR)\" \"-DENABLE_DEVELOPER=$(ENABLE_DEVELOPER)\"" --export-binaries

########
# Clean the build directory
##########

.PHONY: clean

clean:
	rm -f *.o *.a $(EXECUTABLES)
