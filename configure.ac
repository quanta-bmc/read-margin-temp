# Initialization
AC_PREREQ([2.69])
AC_INIT([read-margin-temp], [0.1])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Wno-unused-variable], [CXXFLAGS])

LT_INIT # Required for systemd linking

# Checks for libraries.
AC_CHECK_HEADER(
    nlohmann/json.hpp,
    [],
    [AC_MSG_ERROR([Could not find nlohmann/json.hpp])]
)

# Create configured output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
