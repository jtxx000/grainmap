CXXFLAGS += $(shell pkg-config --cflags $(packages)) -I.
LDFLAGS += $(shell pkg-config --libs $(packages))

define pywrap
_$(1).so: $(1)_wrap.c $(1).c
	$$(CC) -shared -fPIC -I. -DMODULE_HEADER='"'$(1).h'"' $$(CFLAGS) $$(LDFLAGS) $$^ -o $$@

$(1)_wrap.c: $(1).i $(1).h $(1)-create-destroy.i swigtemplate.i
	swig -python -outcurrentdir \
	     -DMODULE_NAME=$(1) \
	     -DMODULE_HEADER=$(1).h \
	     -DCREATE_DESTROY=$(1)-create-destroy.i \
	     -DSWIG_FILE=$(1).i \
	     -o $$@ \
	     $$(VPATH)/swigtemplate.i

$(1)-create-destroy.i: $(1).h
	sed -nr -e 's/^\w+ \*(create_\w+).*/%newobject \1;/p' \
	        -e 's/^void (destroy_\w+)\((\w+).*/%extend \2 {~\2() {\1($$self);}}/p' $$< > $$@

$(1).h: $(1).c
	makeheaders $$<':'$$@
endef

PY = PYTHONPATH=. python2

.PHONY: root
root:
	make -C bin -I.. VPATH=.. -f ../Makefile all

%.h %.cpp: %.lzz
	lzz -hl -sl -hd -sd -o . $<

check-syntax:
	cp $(CHK_SOURCES) bin
	make -s -C bin -I.. VPATH=.. -f ../Makefile $(CHK_SOURCES:.lzz=.cpp)
	cd bin && g++ -c -o /dev/null $(CXXFLAGS) $(LDFLAGS) $(CHK_SOURCES:.lzz=.cpp)
