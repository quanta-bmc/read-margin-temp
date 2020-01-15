ACLOCAL_AMFLAGS = -I m4
AM_DEFAULT_SOURCE_EXT = .cpp
bin_PROGRAMS = read_margin_temp

read_margin_temp_SOURCES = main.cpp
read_margin_temp_LDFLAGS = -lstdc++fs

read_margin_temp_LDADD = \
	libReadMarginTemp.la

noinst_LTLIBRARIES = libReadMarginTemp.la
libReadMarginTemp_la_LDFLAGS = -static
libReadMarginTemp_la_LIBADD = \
	-lstdc++fs \
    $(CODE_COVERAGE_LIBS)

libReadMarginTemp_la_CXXFLAGS = \
	$(CODE_COVERAGE_CXXFLAGS)

libReadMarginTemp_la_SOURCES = \
    json/buildjson.cpp \
    sensor/sensor.cpp \
    util/util.cpp \
	$(BUILT_SOURCES)
