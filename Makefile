# Type Designer Pocket
# make        build the ROM into build/
# make test   build, then run the headless PyBoy tests
# make clean

GBDK   ?= vendor/gbdk
LCC    := $(GBDK)/bin/lcc
ROM    := build/type-designer-pocket.gb
SRC    := $(wildcard src/*.c)
PY     := .venv/bin/python

# Cartridge: MBC5 + RAM + battery (0x1B), one 8 KB save RAM bank, header title.
LCCFLAGS := -Wl-yt0x1B -Wl-ya1 -Wm-yn"TDPOCKET"

all: $(ROM)

$(LCC):
	tools/get-gbdk.sh

$(ROM): $(SRC) | $(LCC)
	@mkdir -p build
	$(LCC) $(LCCFLAGS) -o $@ $(SRC)

$(PY):
	python3 -m venv .venv
	$(PY) -m pip install -q -r tests/requirements.txt

test: $(ROM) | $(PY)
	$(PY) -m unittest discover -s tests -v

clean:
	rm -rf build

.PHONY: all test clean
