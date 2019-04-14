SUBDIRS = lib include
ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = dice
dice_SOURCES = dice.c io.c
dice_CFLAGS = -I$(top_srcdir)/include
dice_LDADD = lib/libdice.la

man1_MANS = dice.1
