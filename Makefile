# The name of the executable to be created
BIN_NAME := mpris-ctl
# Compiler used
CC ?= gcc
# Space-separated pkg-config libraries used by this project
LIBS = dbus-1
# General compiler flags
CFLAGS = -std=c99 -Wall -Wextra -g
# General linker settings
LDFLAGS =
# sources
SOURCES = main.c
# Destination directory
DESTDIR = /
# Install path (bin/ is appended automatically)
INSTALL_PREFIX = usr/local

# Append pkg-config specific libraries if need be
ifneq ($(LIBS),)
	CFLAGS += $(shell pkg-config --cflags $(LIBS))
	LDFLAGS += $(shell pkg-config --libs $(LIBS))
endif

# Version macros
USE_VERSION := false
# If this isn't a git repo or the repo has no tags, git describe will return non-zero
ifeq ($(shell git describe > /dev/null 2>&1 ; echo $$?), 0)
	USE_VERSION := true
	VERSION := $(shell git describe --tags --long --dirty=-git --always )
	VERSION_MAJOR := $(word 1, $(VERSION))
	VERSION_MINOR := $(word 2, $(VERSION))
	VERSION_PATCH := $(word 3, $(VERSION))
	VERSION_REVISION := $(word 4, $(VERSION))
	VERSION_HASH := $(word 5, $(VERSION))
	VERSION_STRING := \
		"$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH).$(VERSION_REVISION)-$(VERSION_HASH)"
	override CFLAGS := $(CFLAGS) \
		-D VERSION_HASH=\"$(VERSION)\"
else
ifneq ($(VERSION_HASH), )
	override CFLAGS := $(CFLAGS) -D VERSION_HASH=\"$(VERSION_HASH)\"
endif
endif

TIME_FILE = $(dir $@).$(notdir $@)_time
START_TIME = date '+%s' > $(TIME_FILE)
END_TIME = read st < $(TIME_FILE) ; \
	$(RM) $(TIME_FILE) ; \
	st=$$((`date '+%s'` - $$st - 86400)) ; \
	echo `date -u -d @$$st '+%H:%M:%S'`

build:
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(SOURCES) -o$(BIN_NAME) 

.PHONY: clean
clean: 
	@echo "Deleting $(BIN_NAME)"
	@$(RM) $(BIN_NAME)

.PHONY: install 
install:
	install $(BIN_NAME) $(DESTDIR)$(INSTALL_PREFIX)/bin

.PHONY: uninstall
uninstall:
	$(RM) $(DESTDIR)$(INSTALL_PREFIX)/bin/$(BIN_NAME)
