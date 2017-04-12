BIN_NAME := mpris-ctl
CC ?= clang
LIBS = dbus-1
COMPILE_FLAGS = -std=c99 -Wall -Wextra
LINK_FLAGS =
RCOMPILE_FLAGS = -D NDEBUG
DCOMPILE_FLAGS = -ggdb -D DEBUG -O1
RLINK_FLAGS =
DLINK_FLAGS =

SOURCES = src/main.c
DESTDIR = /
INSTALL_PREFIX = usr/local

ifneq ($(LIBS),)
	CFLAGS += $(shell pkg-config --cflags $(LIBS))
	LDFLAGS += $(shell pkg-config --libs $(LIBS))
endif

ifeq ($(shell git describe > /dev/null 2>&1 ; echo $$?), 0)
	VERSION := $(shell git describe --tags --long --dirty=-git --always )
endif
ifneq ($(VERSION), )
	override CFLAGS := $(CFLAGS) -D VERSION_HASH=\"$(VERSION)\"
endif

release: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(RCOMPILE_FLAGS)
release: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(RLINK_FLAGS)
debug: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
debug: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)

.PHONY: all
all: release

check_leak: export CC := clang
check_leak: DCOMPILE_FLAGS += -fsanitize=address -fPIE
check_leak: DLINK_FLAGS += -pie
check_leak: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
check_leak: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)

check_thread: export CC := clang
check_thread: DCOMPILE_FLAGS += -fsanitize=thread -lpthread -fPIE
check_thread: DLINK_FLAGS += -pie
check_thread: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
check_thread: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)

check_memory: export CC := clang
check_memory: DCOMPILE_FLAGS += -fsanitize=memory -fPIE
check_memory: DLINK_FLAGS += -pie
check_memory: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
check_memory: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)

check_undefined: export CC := clang
check_undefined: DCOMPILE_FLAGS += -fsanitize=undefined -fPIE
check_undefined: DLINK_FLAGS += -pie
check_undefined: export CFLAGS := $(CFLAGS) $(COMPILE_FLAGS) $(DCOMPILE_FLAGS)
check_undefined: export LDFLAGS := $(LDFLAGS) $(LINK_FLAGS) $(DLINK_FLAGS)

.PHONY: check_leak
check_leak: clean executable run

.PHONY: check_thread
check_thread: clean executable run

.PHONY: check_memory
check_memory: clean executable run

.PHONY: check_undefined
check_undefined: clean executable run

run: $(BIN_NAME)
	./$(BIN_NAME) info %full

.PHONY: release
release: executable

.PHONY: debug
debug: executable

executable:
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCES) $(LDFLAGS) -o$(BIN_NAME)

.PHONY: clean
clean:
	$(RM) $(BIN_NAME)

.PHONY: install
install:
	install $(BIN_NAME) $(DESTDIR)$(INSTALL_PREFIX)/bin

.PHONY: uninstall
uninstall:
	$(RM) $(DESTDIR)$(INSTALL_PREFIX)/bin/$(BIN_NAME)

