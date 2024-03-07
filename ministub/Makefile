CC = smlrcc

all: ministub.exe

ministub.exe: ministub.c
	$(CC) -dosp $< -o _$@
	lfanew -o $@ _$@
	rm _$@
