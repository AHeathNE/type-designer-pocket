# Type Designer Pocket
# make        build the ROM into build/
# make test   build, then run the headless PyBoy tests
# make clean

GBDK   ?= vendor/gbdk
LCC    := $(GBDK)/bin/lcc
ROM    := build/type-designer-pocket.gb
SRC    := $(wildcard src/*.c) build/gen/fonts.c
FONTS  := fonts/pocket-sans.txt fonts/pocket-serif.txt fonts/classic-8x8.txt
HDR    := $(wildcard src/*.h)
PY     := .venv/bin/python

# Cartridge: MBC5 + RAM + battery (0x1B), one 8 KB save RAM bank, header title.
LCCFLAGS := -Wl-yt0x1B -Wl-ya1 -Wm-yn"TDPOCKET"

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

clean:
	rm -rf build

.PHONY: all test clean
