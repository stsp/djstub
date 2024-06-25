TOP = ..
TGT = $(TOP)/ministub.exe
CC = smlrcc
SMFLAGS = -dosp

ifneq ($(SMPATH),)
SMFLAGS += -I $(SMPATH)/v0100/include -SL $(SMPATH)
define P
PATH=$(SMPATH):$(PATH)
endef
endif

all: $(TGT)

$(TGT): ministub.c
	$(P) $(CC) $(SMFLAGS) $< -o $@

clean:
