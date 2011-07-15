AC_PREREQ([2.68])
AC_INIT([grainmap], [0.1], [jtxx000@gmail.com])
AC_CONFIG_SRCDIR([five-color.h])
#AC_CONFIG_HEADERS([config.h])
AC_PROG_CXX
AC_PROG_CC
AC_CHECK_HEADERS([limits.h stdlib.h string.h])
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_UINT32_T
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

AM_INIT_AUTOMAKE
AC_CONFIG_FILES([Makefile])
PKG_CHECK_MODULES([DEPS], [sndfile samplerate cairo python-2.7 pycairo aubio jack gtkmm-3.0 cairomm-1.0])

AC_OUTPUT
