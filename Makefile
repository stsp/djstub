OBJCOPY = objcopy
O_BFDARCH=$(shell $(OBJCOPY) --info 2>/dev/null | head -n 2 | tail -n 1)
ifeq ($(O_BFDARCH),)
O_BFDARCH = elf64-x86-64
endif
PROG = djstubify
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
CFLAGS ?= -Wall -Og -g

all:
	$(MAKE) -C mini
	$(MAKE) $(PROG)

mini/ministub.exe:
	$(MAKE) -C mini

stub.exe: mini/ministub.exe
	cp mini/ministub.exe $@

binstub.o: stub.exe
	$(OBJCOPY) -I binary -O $(O_BFDARCH) \
		--add-section .note.GNU-stack=/dev/null $< $@

$(PROG): stubify.o binstub.o
	$(CC) -o $@ $^

stubify.o: stubify.c
	$(CC) $(CFLAGS) $< -c -o $@

install:
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(PROG) $(DESTDIR)$(BINDIR)
	install -m 0755 djstrip $(DESTDIR)$(BINDIR)
	install -m 0755 djlink $(DESTDIR)$(BINDIR)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(PROG)
	$(RM) $(DESTDIR)$(BINDIR)/djstrip

deb:
	debuild -i -us -uc -b

clean:
	$(MAKE) -C mini clean
	rm -f *.o stub.exe $(PROG)
