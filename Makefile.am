AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4

LIBS_H3 = -lh3

bin_PROGRAMS = split

split_SOURCES = \
	main.c \
	types.h \
	h3.h \
	h3.c \
	wkt/parse.h \
	wkt/parse.c \
	wkt/print.h \
	wkt/print.c

split_LDADD = $(LIBS_H3)
split_CFLAGS = -Wall -Wl,-rpath -Wl,$(prefix)/lib
