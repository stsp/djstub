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
	$(P) $(CC) $(SMFLAGS) $< -o $@

clean:
	$(RM) ministub.exe
