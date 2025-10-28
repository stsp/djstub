TOP = ..
TGT = $(TOP)/ministub.exe
STUBCC = smlrcc
SMFLAGS = -dosp

ifneq ($(SMPATH),)
SMFLAGS += -I $(SMPATH)/v0100/include -SL $(SMPATH)
define P
PATH=$(SMPATH):$(PATH)
endef
endif

all: $(TGT)

$(TGT): ministub.c
	$(P) $(STUBCC) $(SMFLAGS) $< -o $@

clean:
