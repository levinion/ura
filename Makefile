BUILD_DIR ?= build
BUILD_TYPE ?= release
PREFIX ?= /usr
SYSCONFDIR ?= /etc
MESON ?= meson

.PHONY: all build setup test install clean debug

all: build

build: setup
	$(MESON) compile -C "$(BUILD_DIR)"

setup:
	@if [ -f "$(BUILD_DIR)/meson-private/coredata.dat" ]; then \
		$(MESON) setup --reconfigure "$(BUILD_DIR)" \
			--buildtype="$(BUILD_TYPE)" \
			--prefix="$(PREFIX)" \
			--sysconfdir="$(SYSCONFDIR)"; \
	else \
		$(MESON) setup "$(BUILD_DIR)" \
			--buildtype="$(BUILD_TYPE)" \
			--prefix="$(PREFIX)" \
			--sysconfdir="$(SYSCONFDIR)"; \
	fi

test: build
	$(MESON) test -C "$(BUILD_DIR)" --print-errorlogs

install: build
	DESTDIR="$(DESTDIR)" $(MESON) install -C "$(BUILD_DIR)"

clean:
	rm -rf "$(BUILD_DIR)"

debug:
	$(MAKE) BUILD_DIR=build-debug BUILD_TYPE=debug build
