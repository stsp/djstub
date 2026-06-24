TOP = ..
PRG = stub.exe
TGT = $(TOP)/$(PRG)
CC = smlrcc
SMCFLAGS = -dosp
SMLDFLAGS = -dosp
CFILES = stub.c coff.c elf.c util.c dpmi.c ministub.c
OBJECTS = $(CFILES:.c=.o)

all: $(TGT)

$(OBJECTS): $(CFILES) $(wildcard *.h)

%.o: %.c
	$(CC) $(SMCFLAGS) -c -o $@ $<

$(TGT): $(OBJECTS)
	$(CC) $^ $(SMLDFLAGS) -o $@

clean:
	$(RM) *.o *.d $(PRG)
