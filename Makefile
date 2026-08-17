PROJ = djstub2
PROG = djstubify2.py

# on Termux PREFIX is used
ifneq ($(PREFIX),)
prefix := $(PREFIX)
else
prefix ?= /usr/local
endif
bindir ?= $(prefix)/bin
datadir ?= $(prefix)/share
libexecdir ?= $(prefix)/libexec

CFLAGS ?= -Wall -Og -g
STUB = stub.exe
CPPFLAGS += -DDJSTUB_VER=$(VER)

all: $(STUB) djstubify

force:
$(STUB): force
	$(MAKE) -C src ../$@

djstubify: djstubify.in Makefile
	sed -E \
    -e "s,@bindir[@],$(bindir),g" \
    -e "s,@datadir[@],$(datadir),g" \
    -e "s,@libexecdir[@],$(libexecdir),g" \
    -e "s,@prog[@],$(PROG),g" \
    $< >$@
	chmod +x $@

install:
	install -d $(DESTDIR)$(bindir)
	install -d $(DESTDIR)$(datadir)/$(PROJ)
	install -m 0755 djstubify $(DESTDIR)$(bindir)
	install -m 0755 djstrip $(DESTDIR)$(bindir)
	install -m 0755 djlink $(DESTDIR)$(bindir)
	install -m 0755 djelfextract $(DESTDIR)$(bindir)
	install -m 0644 $(STUB) $(DESTDIR)$(datadir)/$(PROJ)
	install -m 0755 $(PROG) $(DESTDIR)$(datadir)/$(PROJ)
	install -m 0644 elf.py $(DESTDIR)$(datadir)/$(PROJ)

uninstall:
	$(RM) $(DESTDIR)$(bindir)/djstubify
	$(RM) $(DESTDIR)$(bindir)/djstrip
	$(RM) $(DESTDIR)$(bindir)/djlink
	$(RM) $(DESTDIR)$(bindir)/djelfextract
	$(RM) -r $(DESTDIR)$(datadir)/$(PROJ)

deb:
	debuild -i -us -uc -b

clean:
	$(MAKE) -C src clean
	rm -f *.o $(STUB) djstubify
