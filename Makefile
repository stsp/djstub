CC = ia16-elf-gcc
CFLAGS = -mcmodel=small -mdosx32 -Os -Wall
LDFLAGS = -mcmodel=small -mdosx32 -li86
OBJCOPY = objcopy
O_BDFARCH=$(shell $(OBJCOPY) --info | head -n 2 | tail -n 1)
PROG = djstubify
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
CFILES = stub.c coff.c elf.c util.c
OBJECTS = $(CFILES:.c=.o)

all: $(PROG)

.INTERMEDIATE: stub.exe

$(OBJECTS): $(CFILES) $(wildcard *.h)

stub.exe: $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o _$@
	lfanew -o $@ _$@
	rm _$@

binstub.o: stub.exe
	$(OBJCOPY) -I binary -O $(O_BDFARCH) \
		--add-section .note.GNU-stack=/dev/null $< $@

$(PROG): stubify.o binstub.o
	cc -o $@ $^

stubify.o: stubify.c
	cc -Wall $< -c -g -o $@

install:
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(PROG) $(DESTDIR)$(BINDIR)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(PROG)

deb:
	debuild -i -us -uc -b

clean:
	rm -f *.o stub.exe $(PROG)
