TOP = ..
PRG = fullstub.exe
TGT = $(TOP)/$(PRG)
CC = smlrcc
CFLAGS = -dosp
LDFLAGS = -dosp
CFILES = stub.c coff.c elf.c util.c dpmi.c
OBJECTS = $(CFILES:.c=.o)

all: $(TGT)

$(OBJECTS): $(CFILES) $(wildcard *.h)

$(TGT): $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	$(RM) *.o *.d $(PRG)
