packages = sndfile samplerate cairo python-2.7 pycairo

CXXFLAGS = -std=c++0x -g
LDFLAGS = -lboost_unit_test_framework

include common.mk

all: five-color-tests

out.png: create-grainmap.py _grainmap.so
	$(PY) $<

grainmap: grainmap.cpp

#$(eval $(call pywrap,grainmap))
