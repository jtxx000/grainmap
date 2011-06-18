packages = sndfile samplerate cairo python-2.7 pycairo

include common.mk

all: out.png

out.png: create-grainmap.py _grainmap.so
	$(PY) $<

$(eval $(call pywrap,grainmap))
