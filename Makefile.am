AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I . -Wall -g

# Static library
HEADER_FILES = \
	split/types.h \
	split/bbox3.h \
	split/h3.h \
	split/parse.h \
	split/print.h \
	split/split.h \
	split/vect3.h
SOURCE_FILES = \
	$(HEADER_FILES) \
	src/bbox3.c \
	src/h3.c \
	src/parse.c \
	src/print.c \
	src/split.c \
	src/vect3.c

lib_LTLIBRARIES = libsplit.la
libsplit_ladir = $(includedir)
nobase_libsplit_la_HEADERS = $(HEADER_FILES)
libsplit_la_SOURCES = $(SOURCE_FILES)
libsplit_la_LIBADD = -lh3 -lm

MYLIBS = libsplit.la -lh3 -lm

# Program
bin_PROGRAMS = split
split_SOURCES = main.c
split_LDADD = $(MYLIBS)

# Tests
TESTS = \
	test_bbox \
	test_bbox1

check_PROGRAMS = $(TESTS)
TEST_SOURCES = test/print.h test/print.c

test_bbox_SOURCES = test/test_bbox.c $(TEST_SOURCES)
test_bbox_LDADD = $(MYLIBS)

test_bbox1_SOURCES = test/test_bbox1.c $(TEST_SOURCES)
test_bbox1_LDADD = $(MYLIBS)
