AM_DEFAULT_SOURCE_EXT = .cpp

AM_CPPFLAGS = $(CODE_COVERAGE_CPPFLAGS)
AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)
AM_CXXFLAGS = $(CODE_COVERAGE_CXXFLAGS)

bin_PROGRAMS = read_margin_temp

read_margin_temp_SOURCES = main.cpp
read_margin_temp_LDADD = \
	libread_margin_temp.la

noinst_LTLIBRARIES = libread_margin_temp.la
libread_margin_temp_la_LDFLAGS = -static

libread_margin_temp_la_SOURCES = \
    json/buildjson.cpp \
    sensor/sensor.cpp \
    util/util.cpp \
    dbus/dbus.cpp
