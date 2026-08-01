PROJ = djstub2
PROG = djstubify2.bin
VER = 8
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

all: $(PROG) $(STUB) djstubify

force:
$(STUB): force
	$(MAKE) -C src ../$@

djstubify: djstubify.in
	sed -E \
    -e "s,@bindir[@],$(bindir),g" \
    -e "s,@datadir[@],$(datadir),g" \
    -e "s,@libexecdir[@],$(libexecdir),g" \
    $< >$@
	chmod +x $@

$(PROG): stubify.o
	$(CC) $(LDFLAGS) -o $@ $^

install:
	install -d $(DESTDIR)$(bindir)
	install -d $(DESTDIR)$(datadir)/$(PROJ)
	install -d $(DESTDIR)$(libexecdir)/$(PROJ)
	install -m 0755 djstubify $(DESTDIR)$(bindir)
	install -m 0755 djstrip $(DESTDIR)$(bindir)
	install -m 0755 djlink $(DESTDIR)$(bindir)
	install -m 0755 djelfextract $(DESTDIR)$(bindir)
	install -m 0644 $(STUB) $(DESTDIR)$(datadir)/$(PROJ)
	install -m 0755 $(PROG) $(DESTDIR)$(libexecdir)/$(PROJ)

uninstall:
	$(RM) $(DESTDIR)$(bindir)/djstubify
	$(RM) $(DESTDIR)$(bindir)/djstrip
	$(RM) $(DESTDIR)$(bindir)/djlink
	$(RM) $(DESTDIR)$(bindir)/djelfextract
	$(RM) -r $(DESTDIR)$(datadir)/$(PROJ)
	$(RM) -r $(DESTDIR)$(libexecdir)/$(PROJ)

deb:
	debuild -i -us -uc -b

clean:
	$(MAKE) -C src clean
	rm -f *.o $(STUB) $(PROG) djstubify
