packages = sndfile samplerate cairo python-2.7 pycairo aubio

CXXFLAGS = -std=c++0x -g
LDFLAGS = -lboost_unit_test_framework

include common.mk

all: five-color-tests grainmap

out.png: create-grainmap.py _grainmap.so
	$(PY) $<

grainmap: grainmap.cpp hilbert.c five-color.cpp

#$(eval $(call pywrap,grainmap))
