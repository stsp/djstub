PROG = djstubify
VER = 7
prefix ?= /usr/local
BINDIR ?= $(prefix)/bin
CFLAGS ?= -Wall -Og -g
# set to mini or full
STYPE = mini
STUB = $(STYPE)stub.exe
STUB_S = $(STYPE)stub.S
CPPFLAGS += -DDJSTUB_VER=$(VER) \
  -D_binary_stub_exe_start=_binary_$(STYPE)stub_exe_start \
  -D_binary_stub_exe_end=_binary_$(STYPE)stub_exe_end \
  -D_binary_stub_exe_size=_binary_$(STYPE)stub_exe_size

all: $(PROG)

.PHONY: $(STYPE)
$(STUB): $(STYPE)
	$(MAKE) -C $<

binstub.o: $(STUB_S) $(STUB)
	$(CC) -c -o $@ $<

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
	$(MAKE) -C $(STYPE) clean
	rm -f *.o $(STUB) $(PROG)
