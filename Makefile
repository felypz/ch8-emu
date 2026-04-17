VERSION = 0.2
PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin

OS := $(shell uname -s)

CC = gcc
ifeq ($(OS), Darwin)
	CC = clang
	HOMEBREW_PREFIX := $(shell brew --prefix 2>/dev/null || echo /opt/homebrew)
	export PKG_CONFIG_PATH := $(HOMEBREW_PREFIX)/lib/pkgconfig:$(PKG_CONFIG_PATH)
endif

INCS = $(shell pkg-config --cflags sdl3)
LIBS = $(shell pkg-config --libs sdl3)

WARNINGS   = -Wall -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes
BASE_FLAGS = $(WARNINGS) -std=c11 -D_XOPEN_SOURCE=600 -DVERSION=\"$(VERSION)\" $(INCS)
CFLAGS     = -O2 $(BASE_FLAGS)
DBGFLAGS   = -O0 -g $(BASE_FLAGS)
LDFLAGS    = $(LIBS) -lm

SRC = ch8.c config.c main.c sdl.c
OBJ = $(SRC:.c=.o)

all: ch8

$(OBJ): ch8.h config.h sdl.h

ch8: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

debug: $(SRC)
	$(CC) $(DBGFLAGS) -o ch8-debug $(SRC) $(LDFLAGS)

format:
	clang-format -i $(SRC) ch8.h config.h sdl.h

clean:
	rm -f ch8 ch8-debug $(OBJ) ch8-$(VERSION).tar.gz

dist: clean
	mkdir -p ch8-$(VERSION)
	cp -R LICENSE Makefile README.md ch8.c ch8.h config.c config.h main.c sdl.c sdl.h ch8-$(VERSION)
	tar -cf - ch8-$(VERSION) | gzip > ch8-$(VERSION).tar.gz
	rm -rf ch8-$(VERSION)

install: ch8
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -f ch8 $(DESTDIR)$(BINDIR)/ch8
	chmod 755 $(DESTDIR)$(BINDIR)/ch8

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/ch8

.PHONY: all clean debug dist format install uninstall
