CC = ia16-elf-gcc
CFLAGS = -mcmodel=small -mdosx -Wall
LDFLAGS = -mcmodel=small -mdosx
OBJCOPY = objcopy
O_BDFARCH=$(shell $(OBJCOPY) --info | head -n 2 | tail -n 1)

all: dosemu2-stubify

.INTERMEDIATE: stub.exe

stub.exe: stub.c
	$(CC) $(CFLAGS) $< -li86 -o $@

stub.o: stub.exe
	$(OBJCOPY) -I binary -O $(O_BDFARCH) \
		--add-section .note.GNU-stack=/dev/null $< $@

dosemu2-stubify: stubify.o stub.o
	cc -o $@ $^

stubify.o: stubify.c
	cc -Wall $< -c -o $@

clean:
	rm -f *.o stub.exe dosemu2-stubify
