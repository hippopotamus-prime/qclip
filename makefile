#############################################################################
## QClip
## Copyright 2006 Aaron Curtis
##
## QClip is a clipboard extender for Windows.  It monitors the standard
## Windows clipboard and pushes any data placed there into a queue.
## Users can then pop data from either end of the queue when needed  or
## select items arbitrarily from a menu.  In either case, the program
## performs a "paste" operation automatically.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this file; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#############################################################################

SOURCE   =  Clipboard.c ClipFile.c ClipQueue.c FormatSettings.c GeneralSettings.c \
            KeySettings.c QClip.c RecentFiles.c Settings.c About.c main.c \
            DateTimeWrapper.c

OBJECTS  = $(SOURCE:.c=.o)
RESOURCE = resource.res
EXE      = QClip.exe
CC       = gcc
RC       = windres
CFLAGS   = -fexpensive-optimizations -O2 -Wall
LFLAGS   = -fexpensive-optimizations -O2 -mwindows -lcomctl32 -s
RFLAGS   = --input-format=rc -O coff

all: $(EXE) strip

unicode: CFLAGS = -fexpensive-optimizations -O2 -Wall -DUNICODE
unicode: LFLAGS = -fexpensive-optimizations -O2 -mwindows -lcomctl32 -s -DUNICODE
unicode: RFLAGS = --input-format=rc -O coff -DUNICODE
unicode: $(EXE) strip

debug: CFLAGS = -g3 -D__DEBUG__
debug: LFLAGS = -g3 -mwindows -lcomctl32
debug: $(EXE)

$(EXE): $(OBJECTS) $(RESOURCE)
	$(CC) $(OBJECTS) $(RESOURCE) $(LFLAGS) -o $@

strip:
	strip --strip-all $(EXE)

%.res: %.rc %.h
	$(RC) -i $< -o $@ $(RFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(RESOURCE) $(EXE)

cleaner:
	rm -f $(OBJECTS) $(RESOURCE) $(EXE)

