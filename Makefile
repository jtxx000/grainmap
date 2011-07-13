packages = sndfile samplerate cairo python-2.7 pycairo aubio jack gtkmm-2.4 cairomm-1.0

CXXFLAGS = -std=c++0x -g
LDFLAGS = -lboost_unit_test_framework

include common.mk

all: graingui

out.png: create-grainmap.py _grainmap.so
	$(PY) $<

#graingui: graingui.cpp grainmap.cpp grainmap.h grainaudio.cpp hilbert.c five-color.cpp
graingui: graingui.o grainmap.o grainaudio.o hilbert.o five-color.o

grainmap.o: grainmap.h

#$(eval $(call pywrap,grainmap))
