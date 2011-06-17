packages = alsa x11 freetype2 xext gl

#CXX=g++
CPPFLAGS = -DLINUX=1 $(shell pkg-config --cflags $(packages)) -g
LDFLAGS = -ljuce $(shell pkg-config --libs $(packages))

all: out.png

out.png: main
	./main

main: main.cpp
	g++ $(CPPFLAGS) -o $@ $< $(LDFLAGS)
