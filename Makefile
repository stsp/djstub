OBJCOPY = objcopy
O_BFDARCH=$(shell $(OBJCOPY) --info 2>/dev/null | head -n 2 | tail -n 1)
ifeq ($(O_BFDARCH),)
O_BFDARCH = elf64-x86-64
endif
PROG = djstubify
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
CFLAGS ?= -Wall -Og -g
# set to mini or full
STYPE = mini
STUB = $(STYPE)stub.exe
CFLAGS += \
  -D_binary_stub_exe_start=_binary_$(STYPE)stub_exe_start \
  -D_binary_stub_exe_end=_binary_$(STYPE)stub_exe_end \
  -D_binary_stub_exe_size=_binary_$(STYPE)stub_exe_size

all: $(PROG)

.PHONY: $(STYPE)
$(STUB): $(STYPE)
	$(MAKE) -C $<

binstub.o: $(STUB)
	$(OBJCOPY) -I binary -O $(O_BFDARCH) \
		--add-section .note.GNU-stack=/dev/null $< $@

$(PROG): stubify.o binstub.o
	$(CC) $(LDFLAGS) -o $@ $^

install:
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(PROG) $(DESTDIR)$(BINDIR)
	install -m 0755 djstrip $(DESTDIR)$(BINDIR)
	install -m 0755 djlink $(DESTDIR)$(BINDIR)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(PROG)
	$(RM) $(DESTDIR)$(BINDIR)/djstrip
	$(RM) $(DESTDIR)$(BINDIR)/djlink

deb:
	debuild -i -us -uc -b

clean:
	$(MAKE) -C $(STYPE) clean
	rm -f *.o $(STUB) $(PROG)
