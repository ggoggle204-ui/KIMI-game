# Desktop build (SDL2) - handy for testing the game logic before deploying to PS3.
CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++11 -Isrc $(shell sdl2-config --cflags)
LDLIBS   ?= $(shell sdl2-config --libs)

SRC := $(wildcard src/*.cpp) src/sdl/main_sdl.cpp

skycircuit: $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LDLIBS)

ps3:
	$(MAKE) -f Makefile.ps3

clean:
	rm -f skycircuit

.PHONY: clean ps3
