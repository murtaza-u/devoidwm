LIBS = -lX11
CFLAGS += -std=c99 -Wall -Wextra -pedantic -Os
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
CC ?= gcc

all: devoid

devoid: devoid.o
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 devoid $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/devoid

clean:
	rm -f devoid *.o

.PHONY: all install uninstall clean
