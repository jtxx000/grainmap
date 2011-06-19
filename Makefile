packages = sndfile samplerate cairo python-2.7 pycairo

CXXFLAGS = -std=c++0x

include common.mk

all: out.png

out.png: create-grainmap.py _grainmap.so
	$(PY) $<

grainmap: grainmap.cpp

#$(eval $(call pywrap,grainmap))
