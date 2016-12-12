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
CFLAGS	    = -c -municode
LDFLAGS	    = -static  -mwindows -municode
LDLIBS	    = -lcomctl32 -lmpr -lcrypt32 -ladvapi32 -luser32 -lkernel32 \
              -lshlwapi

################################################################################
# Git versions
GIT_VERSION = $(shell git describe --dirty --always)
GIT_TAG     = $(shell git describe --abbrev=0 --tags)
GIT_COMMITS = $(shell git rev-list --all --count)

# Git defines
PROJ_DEFINES := -DPROJECT_NAME=\"$(PROJ)\"       \
		-DPROJECT_COMMITS=$(GIT_COMMITS) \
		-DPROJECT_VER=\"$(GIT_TAG)\"     \
	        -DPROJECT_GIT=\"$(GIT_VERSION)\"

# Objects and outputs
OBJ = $(RES:.rc=.o) $(SRC:.c=.o)
EXECUTABLE = $(addsuffix .exe,$(PROJ)) 

# Debug flags
ifeq ($(DBG),y)
    CFLAGS  += -D_DEBUG
    DCFLAGS  = -ggdb -Wall
    DLDFLAGS = -ggdb
else
    DCFLAGS  =
    DLDFLAGS =
endif
################################################################################

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ)
	$(CC) $(LDFLAGS) $(DLDFLAGS) $(OBJ)  -o $@ $(LDLIBS)

%.o: %.c	
	$(CC) $(CFLAGS) $(DCFLAGS) $< -o $@

defs.o: defs.c
	$(CC) $(CFLAGS) $(DCFLAGS) $(PROJ_DEFINES) $< -o $@
	
%.o: %.rc	
	$(WINDRES) $(CFLAGS) $(subst \",\\\",$(PROJ_DEFINES)) -i $< -o $@

clean:
	$(RM) $(EXECUTABLE) $(SRC:.c=.o) $(RES:.rc=.o)

