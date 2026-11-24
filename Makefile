# Type Designer Pocket
# make        build the ROM into build/
# make test   build, then run the headless PyBoy tests
# make web    the website, with the ROM, in build/web (serve that folder)
# make clean

GBDK   ?= vendor/gbdk
LCC    := $(GBDK)/bin/lcc
ROM    := build/type-designer-pocket.gb
SRC    := $(wildcard src/*.c) build/gen/fonts.c
FONTS  := fonts/pocket-sans.txt fonts/pocket-serif.txt fonts/classic-8x8.txt
HDR    := $(wildcard src/*.h)
PY     := .venv/bin/python

# Cartridge: MBC5 + RAM + battery (0x1B), one 8 KB save RAM bank, header title; symbol file for tests.
# ROM is 32 KB. The game never switches ROM banks, so bank 1 stays mapped at 0x4000 and files marked
# `#pragma bank 1` (the fonts, challenge, type test) are called and read like bank 0. Past 32 KB, real
# banking (BANKED calls, SWITCH_ROM for data) will be needed. `vendor/gbdk/bin/romusage build/*.noi -a`
# shows how full each bank is.
LCCFLAGS := -Wl-yt0x1B -Wl-ya1 -Wm-yn"TDPOCKET" -Wl-j   # -Wl-j: symbol file (.noi) for the tests

all: $(ROM)

# vendor/gbdk/.version records the installed toolchain; changing the pin re-downloads and rebuilds
$(GBDK)/.version: tools/get-gbdk.sh
	tools/get-gbdk.sh
	@touch $@

build/gen/fonts.c: $(FONTS) tools/font2c.py
	python3 tools/font2c.py $@ pocket_sans=fonts/pocket-sans.txt pocket_serif=fonts/pocket-serif.txt classic_8x8=fonts/classic-8x8.txt

$(ROM): $(SRC) $(HDR) $(GBDK)/.version
	@mkdir -p build
	$(LCC) $(LCCFLAGS) -Isrc -o $@ $(SRC)

$(PY):
	python3 -m venv .venv
	$(PY) -m pip install -q -r tests/requirements.txt

test: $(ROM) | $(PY)
	$(PY) -m unittest discover -s tests -v

web: $(ROM)
	rm -rf build/web
	cp -r web build/web
	cp $(ROM) build/web/

clean:
	rm -rf build

.PHONY: all test web clean
