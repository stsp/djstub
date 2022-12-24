CC = ia16-elf-gcc
CFLAGS = -mcmodel=small -mdosx -Os -Wall
LDFLAGS = -mcmodel=small -mdosx
OBJCOPY = objcopy
O_BDFARCH=$(shell $(OBJCOPY) --info | head -n 2 | tail -n 1)
PROG = dosemu2-stubify
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

all: $(PROG)

.INTERMEDIATE: stub.exe

stub.exe: stub.c
	$(CC) $(CFLAGS) $< -li86 -o $@

stub.o: stub.exe
	$(OBJCOPY) -I binary -O $(O_BDFARCH) \
		--add-section .note.GNU-stack=/dev/null $< $@

dosemu2-stubify: stubify.o stub.o
	cc -o $@ $^

stubify.o: stubify.c
	cc -Wall $< -c -g -o $@

install:
	install -D -t $(DESTDIR)$(BINDIR) -m 0755 $(PROG)

deb:
	debuild -i -us -uc -b

clean:
	rm -f *.o stub.exe $(PROG)
