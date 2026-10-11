# LisaPaint -- a pixel painting tool for the Lisa Office System, built with
# lisa-c (https://github.com/pappadf/lisa-c).
#
#   make            build build/lisapaint.dc42 (a floppy with the tool)
#   make toolchain  download lisa-c's prebuilt compiler (once)
#   make run        install and open it on the LOS desktop in granny-smith;
#                   screenshots in build/emu/
#   make demo       ... and paint something with the tools (build/demo/)
#   make serve      run it interactively in granny-smith's web build: prints a
#                   link to open in a browser (lisa-c/emu/serve.sh)
#   make export FLOPPY=saved.dc42 [FORMAT=png|mac|lpnt]
#                   the pictures on a floppy image -> build/export/
#   make clean
#
# PICTURES (PNG or MacPaint files, 576 x 720) go on the floppy as LisaPaint
# documents named after the files: make PICTURES="demo/astronaut.png old.mac".
#
# LISA_C is the lisa-c checkout: the submodule (git clone --recursive), or a
# lisa-c next to this directory.
LISA_C   ?= $(if $(wildcard lisa-c/Makefile),lisa-c,../lisa-c)
TOOLNUM  ?= 21
TOOLNAME ?= LisaPaint
SRC      := $(CURDIR)/src
PICTURES ?= demo/astronaut.png
FORMAT   ?= png
# a document per picture: build/pictures/<name>.lpnt, named <Name> on the desktop
PICS     := $(foreach p,$(PICTURES),build/pictures/$(basename $(notdir $(p))).lpnt)
docname   = $(shell echo '$(basename $(notdir $(1)))' | sed 's/^./\U&/')
DISKARGS := $(foreach p,$(PICTURES),--document '$(call docname,$(p))' $(TOOLNUM) PIC=$(CURDIR)/build/pictures/$(basename $(notdir $(p))).lpnt)
LISA_MAKE = $(MAKE) -C $(LISA_C) $(if $(ENTRIES),ENTRIES=$(abspath $(ENTRIES)))

all: build/lisapaint.dc42

build/lisapaint.dc42: $(wildcard src/*.c src/*.h src/*.phrase) $(PICS)
	@[ -f $(LISA_C)/Makefile ] || { echo "no lisa-c at $(LISA_C): git submodule update --init, or set LISA_C" >&2; exit 1; }
	$(LISA_MAKE) PROG=lisapaint SRCDIR=$(SRC) TOOLNUM=$(TOOLNUM) TOOLNAME='$(TOOLNAME)' DISKARGS="$(DISKARGS)" tool
	mkdir -p build
	cp $(LISA_C)/build/lisapaint-tool.dc42 $@

define picture_rule
build/pictures/$(basename $(notdir $(1))).lpnt: $(1) tools/lpconv.py
	@mkdir -p build/pictures
	python3 -I tools/lpconv.py $$< $$@
endef
$(foreach p,$(PICTURES),$(eval $(call picture_rule,$(p))))

export:
	@[ -n "$(FLOPPY)" ] || { echo "make export FLOPPY=disk.dc42 [FORMAT=png|mac|lpnt]" >&2; exit 1; }
	LISA_C=$(abspath $(LISA_C)) python3 -I tools/lpconv.py export $(FLOPPY) build/export --format $(FORMAT)

toolchain:
	$(MAKE) -C $(LISA_C) toolchain

run: build/lisapaint.dc42
	$(LISA_C)/emu/run-los-tool.sh $< build/emu

serve: build/lisapaint.dc42
	$(LISA_C)/emu/serve.sh $<

demo: build/lisapaint.dc42
	LISA_C=$(abspath $(LISA_C)) tests/demo.sh $< build/demo

clean:
	rm -rf build

.PHONY: all toolchain run demo serve export clean
