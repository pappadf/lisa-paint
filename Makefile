# LisaPaint -- a pixel painting tool for the Lisa Office System, built with
# lisa-c (https://github.com/pappadf/lisa-c).
#
#   make            build build/lisapaint.dc42 (a floppy with the tool)
#   make run        install and open it on the LOS desktop in granny-smith;
#                   screenshots in build/emu/
#   make demo       ... and paint something with the tools (build/demo/)
#   make clean
#
# LISA_C is a lisa-c checkout (with its compiler: `make -C $(LISA_C) toolchain`).
LISA_C   ?= ../lisa-c
TOOLNUM  ?= 21
TOOLNAME ?= LisaPaint
SRC      := $(CURDIR)/src

all: build/lisapaint.dc42

build/lisapaint.dc42: $(wildcard src/*.c src/*.h src/*.phrase)
	$(MAKE) -C $(LISA_C) PROG=lisapaint SRCDIR=$(SRC) TOOLNUM=$(TOOLNUM) TOOLNAME='$(TOOLNAME)' tool
	mkdir -p build
	cp $(LISA_C)/build/lisapaint-tool.dc42 $@

run: build/lisapaint.dc42
	$(LISA_C)/emu/run-los-tool.sh $< build/emu

demo: build/lisapaint.dc42
	tests/demo.sh $< build/demo

clean:
	rm -rf build

.PHONY: all run demo clean
