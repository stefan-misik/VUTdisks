#! /bin/make -f
# Makefile to build VUTdisks application using cygwin with mingw compiler
# or mingw itself
#
# Author: Stefan Misik (mail@stefanmisik.eu)

# Set commands names
ifeq ($(shell uname -o),Cygwin)
	CC	= i686-w64-mingw32-gcc
	WINDRES = i686-w64-mingw32-windres
else
	CC	= gcc
	WINDRES = windres
endif

# Project settings
PROJ	= VUTdisks
SRC	= vut_disks.c registry.c disk_mapper.c win_tile_manifest_gen.c  \
          defs.c about_dialog.c
RES	= resource.rc

# Compile flags
RESFLAGS    =
CFLAGS	    = -c -municode
LDFLAGS	    = -static  -mwindows -municode
LDLIBS	    = -lcomctl32 -lmpr -lcrypt32 -ladvapi32 -luser32 -lkernel32 \
              -lshlwapi
# Number to subtract from the last git commits count
LAST_COMMIT = 32

################################################################################
# Git versions
GIT_VERSION = $(shell git describe --dirty --always)
GIT_TAG     = $(shell git describe --abbrev=0 --tags)
GIT_COMMITS = $$(( $(shell git rev-list --all --count) - $(LAST_COMMIT) ))

# Git defines
PROJ_DEFINES := -DPROJECT_NAME=\"$(PROJ)\"		    \
		-DPROJECT_COMMITS=$(GIT_COMMITS)	    \
		-DPROJECT_LAST_RELEASE=\"$(GIT_TAG)\"	    \
	        -DPROJECT_GIT=\"$(GIT_VERSION)\"	    \
		-DPROJECT_LAST_COMMIT=$(LAST_COMMIT)

# Objects and outputs
OBJ = $(RES:.rc=.o) $(SRC:.c=.o)
EXECUTABLE = $(addsuffix .exe,$(PROJ))

# Debug flags
ifeq ($(DBG),y)
    CFLAGS   += -D_DEBUG -ggdb -Wall
    RESFLAGS += -D_DEBUG
    LDFLAGS  += -ggdb
endif
################################################################################

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ)  -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

defs.o: defs.c
	$(CC) $(CFLAGS) $(PROJ_DEFINES) $< -o $@

%.o: %.rc
	$(WINDRES) $(RESFLAGS) $(subst \",\\\",$(PROJ_DEFINES)) -i $< -o $@

clean:
	$(RM) $(EXECUTABLE) $(SRC:.c=.o) $(RES:.rc=.o)

