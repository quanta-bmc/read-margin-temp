AM_DEFAULT_SOURCE_EXT = .cpp

AM_CPPFLAGS = $(CODE_COVERAGE_CPPFLAGS)
AM_CFLAGS = $(CODE_COVERAGE_CFLAGS)
AM_CXXFLAGS = $(CODE_COVERAGE_CXXFLAGS)

bin_PROGRAMS = read-margin-temp

read-margin-temp_SOURCES = main.cpp
read-margin-temp_LDADD = \
	libread-margin-temp.la

noinst_LTLIBRARIES = libread-margin-temp.la
libread-margin-temp_la_LDFLAGS = -static

libread-margin-temp_la_SOURCES = \
    json/buildjson.cpp \
    sensor/sensor.cpp \
    util/util.cpp
