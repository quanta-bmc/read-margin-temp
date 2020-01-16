AC_INIT([read-margin-temp], [1.0])
LT_INIT
AM_INIT_AUTOMAKE([subdir-objects -Wall -Wno-portability -Werror foreign dist-xz])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])

AC_CONFIG_FILES([
    Makefile
])
AC_OUTPUT
