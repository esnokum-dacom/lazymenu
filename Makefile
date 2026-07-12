CFLAGS += -std=c99 -O2 -Wall -Wextra -pedantic
CLIBS  += -lX11 $(shell pkg-config --cflags --libs xft)
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
CC     ?= gcc

all: lm

config.h:
	cp config.def.h config.h

lm: lm.c config.h Makefile
	$(CC) $(CFLAGS) -o lm lm.c $(CLIBS) $(LDFLAGS)

install: all
	install -Dm755 lm $(DESTDIR)$(BINDIR)/lm

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/lm

clean:
	rm -f lm config.h

.PHONY: all install uninstall clean
