######################################################################
# makefile
#
# Builds the RTK support programs
######################################################################

##########
# Source files
##########

EXECUTABLES  = Fonts
EXECUTABLES += Icons

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

########
# Clean the build directory
##########

.PHONY: clean

clean:
	rm -f *.o *.a $(EXECUTABLES)
