%{
#include <pycairo.h>

static Pycairo_CAPI_t *Pycairo_CAPI;
%}

%init %{
  Pycairo_IMPORT;
%}

%typemap(in) cairo_t* {
  assert(PyObject_IsInstance($input, (PyObject *) &PycairoContext_Type));
  $1 = PycairoContext_GET($input);
}
