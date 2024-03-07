CC = smlrcc
SMFLAGS = -dosp

ifneq ($(SMPATH),)
SMFLAGS += -I $(SMPATH)/v0100/include -SL $(SMPATH)
define P
PATH=$(SMPATH):$(PATH)
endef
endif

all: ministub.exe

ministub.exe: ministub.c
	$(P) $(CC) $(SMFLAGS) $< -o _$@
	lfanew -o $@ _$@
	rm _$@

clean:
	$(RM) ministub.exe
