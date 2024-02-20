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

$(OBJECTS): $(CFILES) $(wildcard *.h)

ifneq ($(shell $(CC) --version 2>/dev/null),)
stub.exe: $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o _$@
	lfanew -o $@ _$@
	rm _$@
else
stub.exe: binstub/stub.exe
	cp $< $@
endif

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
	install -m 0755 djstrip $(DESTDIR)$(BINDIR)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(PROG)
	$(RM) $(DESTDIR)$(BINDIR)/djstrip

deb:
	debuild -i -us -uc -b

clean:
	rm -f *.o stub.exe $(PROG)
