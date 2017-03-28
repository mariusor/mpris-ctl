BIN_NAME := mpris-ctl
CC ?= gcc
LIBS = dbus-1
CFLAGS = -std=c99 -Wall -Wextra -g
LDFLAGS =
SOURCES = src/main.c
DESTDIR = /
INSTALL_PREFIX = usr/local

ifneq ($(LIBS),)
	CFLAGS += $(shell pkg-config --cflags $(LIBS))
	LDFLAGS += $(shell pkg-config --libs $(LIBS))
endif

ifeq ($(shell git describe > /dev/null 2>&1 ; echo $$?), 0)
	VERSION := $(shell git describe --tags --long --dirty=-git --always )
	override CFLAGS := $(CFLAGS) -D VERSION_HASH=\"$(VERSION)\"
endif
ifneq ($(VERSION), )
	override CFLAGS := $(CFLAGS) -D VERSION_HASH=\"$(VERSION)\"
endif

build:
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(SOURCES) -o$(BIN_NAME)

.PHONY: clean
clean:
	@$(RM) $(BIN_NAME)

.PHONY: install
install:
	install $(BIN_NAME) $(DESTDIR)$(INSTALL_PREFIX)/bin

.PHONY: uninstall
uninstall:
	$(RM) $(DESTDIR)$(INSTALL_PREFIX)/bin/$(BIN_NAME)
