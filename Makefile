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
#   make clean
#
# LISA_C is the lisa-c checkout: the submodule (git clone --recursive), or a
# lisa-c next to this directory.
LISA_C   ?= $(if $(wildcard lisa-c/Makefile),lisa-c,../lisa-c)
TOOLNUM  ?= 21
TOOLNAME ?= LisaPaint
SRC      := $(CURDIR)/src
LISA_MAKE = $(MAKE) -C $(LISA_C) $(if $(ENTRIES),ENTRIES=$(abspath $(ENTRIES)))

all: build/lisapaint.dc42

build/lisapaint.dc42: $(wildcard src/*.c src/*.h src/*.phrase)
	@[ -f $(LISA_C)/Makefile ] || { echo "no lisa-c at $(LISA_C): git submodule update --init, or set LISA_C" >&2; exit 1; }
	$(LISA_MAKE) PROG=lisapaint SRCDIR=$(SRC) TOOLNUM=$(TOOLNUM) TOOLNAME='$(TOOLNAME)' tool
	mkdir -p build
	cp $(LISA_C)/build/lisapaint-tool.dc42 $@

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

.PHONY: all toolchain run demo serve clean
