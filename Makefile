PROG = djstubify
VER = 8
# on Termux PREFIX is used
ifneq ($(PREFIX),)
prefix := $(PREFIX)
else
prefix ?= /usr/local
endif
BINDIR ?= $(prefix)/bin
CFLAGS ?= -Wall -Og -g
STUB = stub.exe
STUB_S = stub.S
CPPFLAGS += -DDJSTUB_VER=$(VER)

all: $(PROG)

force:
$(STUB): force
	$(MAKE) -C src ../$@

binstub.o: $(STUB_S) $(STUB)
	$(CC) -c -o $@ $<

stubify.o: Makefile

$(PROG): stubify.o binstub.o
	$(CC) $(LDFLAGS) -o $@ $^

install:
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(PROG) $(DESTDIR)$(BINDIR)
	install -m 0755 djstrip $(DESTDIR)$(BINDIR)
	install -m 0755 djlink $(DESTDIR)$(BINDIR)
	install -m 0755 djelfextract $(DESTDIR)$(BINDIR)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(PROG)
	$(RM) $(DESTDIR)$(BINDIR)/djstrip
	$(RM) $(DESTDIR)$(BINDIR)/djlink
	$(RM) $(DESTDIR)$(BINDIR)/djelfextract

deb:
	debuild -i -us -uc -b

clean:
	$(MAKE) -C src clean
	rm -f *.o $(STUB) $(PROG)
