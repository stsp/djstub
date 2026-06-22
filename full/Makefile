TOP = ..
PRG = fullstub.exe
TGT = $(TOP)/$(PRG)
CC = ia16-elf-gcc
CFLAGS = -mcmodel=small -mdosx32 -Os -Wall
LDFLAGS = -mcmodel=small -mdosx32 -li86
CFILES = stub.c coff.c elf.c util.c
OBJECTS = $(CFILES:.c=.o)

all: $(TGT)

$(OBJECTS): $(CFILES) $(wildcard *.h)

ifneq ($(shell $(CC) --version 2>/dev/null),)
$(PRG): $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@
$(TGT): $(PRG)
	lfanew -o $@ $<
else
$(error ia16-gcc not installed)
endif

clean:
	$(RM) *.o *.d $(PRG)
