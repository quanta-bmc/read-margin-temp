ACLOCAL_AMFLAGS = -I m4
AM_DEFAULT_SOURCE_EXT = .cpp
bin_PROGRAMS = read_margin_temp

read_margin_temp_SOURCES = main.cpp
read_margin_temp_LDFLAGS = -lstdc++fs

read_margin_temp_LDADD = \
	libread_margin_temp.la

noinst_LTLIBRARIES = libread_margin_temp.la
libread_margin_temp_la_LDFLAGS = -static
libread_margin_temp_la_LIBADD = \
	-lstdc++fs \

libread_margin_temp_la_CXXFLAGS = \
	-flto

libread_margin_temp_la_SOURCES = \
    json/buildjson.cpp \
    sensor/sensor.cpp \
    util/util.cpp \
	$(BUILT_SOURCES)
