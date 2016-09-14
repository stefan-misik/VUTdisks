# Author: Stefan Misik (mail@stefanmisik.eu)


ifeq ($(shell uname -o),Cygwin)
	CC	= i686-w64-mingw32-gcc
	WINDRES = i686-w64-mingw32-windres
else
	CC	= gcc
	WINDRES = windres
endif
    
    
PROJ	= VUTdisks
SRC	= vut_disks.c registry.c disk_mapper.c win_tile_manifest_gen.c
RES	= resource.rc

CFLAGS	    = -c -municode
LDFLAGS	    = -static  -mwindows -municode
LDLIBS	    = -lcomctl32 -lmpr -lcrypt32 -ladvapi32 -luser32 -lkernel32 -lshlwapi

################################################################################

OBJ = $(RES:.rc=.o) $(SRC:.c=.o)
EXECUTABLE = $(addsuffix .exe,$(PROJ)) 

ifeq ($(DBG),y)
    CFLAGS  += -D_DEBUG -ggdb -Wall
    LDFLAGS += -ggdb
endif


.PHONY: all clean



all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ)  -o $@ $(LDLIBS)

%.o: %.c	
	$(CC) $(CFLAGS) $< -o $@
	
%.o: %.rc	
	$(WINDRES) $< -o $@

clean:
	$(RM) $(EXECUTABLE) $(SRC:.c=.o) $(RES:.rc=.o)

