OBJCOPY = objcopy
O_BFDARCH=$(shell $(OBJCOPY) --info | head -n 2 | tail -n 1)
PROG = djstubify
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

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
	$(MAKE) -C mini clean
	rm -f *.o stub.exe $(PROG)
