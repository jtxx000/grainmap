packages = sndfile samplerate cairo python-2.7 pycairo aubio jack gtkmm-2.4

CXXFLAGS = -std=c++0x -g
LDFLAGS = -lboost_unit_test_framework

include common.mk

all: five-color-tests graingui

out.png: create-grainmap.py _grainmap.so
	$(PY) $<

graingui: graingui.cpp grainmap.cpp grainaudio.cpp hilbert.c five-color.cpp

#$(eval $(call pywrap,grainmap))
