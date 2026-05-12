BUILD_TYPE ?= Release

PROTOCOL_FILES = $(wildcard protocols/*.xml)
PROTOCOL_HEADERS = $(patsubst protocols/%.xml,include/protocols/%-protocol.h,$(PROTOCOL_FILES))

ifneq ($(shell command -v ninja),)
    NINJA := -G Ninja 
endif

ifneq ($(shell command -v sccache),)
    LAUNCHER := -DCMAKE_C_COMPILER_LAUNCHER=sccache -DCMAKE_CXX_COMPILER_LAUNCHER=sccache
endif

sudo-install:
	sudo $(MAKE) clean
	$(MAKE) build
	sudo $(MAKE) install

install:
	install -Dm755 ./build/ura -t $(DESTDIR)/usr/bin/
	install -Dm644 ./assets/ura.desktop -t $(DESTDIR)/usr/share/wayland-sessions/
	install -Dm644 ./assets/init.lua -t $(DESTDIR)/etc/ura/
	install -Dm644 ./assets/completions/zsh/* -t $(DESTDIR)/usr/share/zsh/site-functions/
	cp -r ura $(DESTDIR)/usr/share/
	install -Dm755 ./tools/scripts/* -t $(DESTDIR)/usr/bin/
	install -Dm755 ./tools/ura-shell/target/release/ura-shell -t $(DESTDIR)/usr/bin/

init: CMakeLists.txt include/protocols $(PROTOCOL_HEADERS) src/ipc.c cmake/CPM.cmake
	cmake -B build \
		$(NINJA) \
		$(LAUNCHER) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: init
	cmake --build build -j$(shell nproc)
	$(MAKE) build-tools

build-tools:
	cd ./tools/ura-shell && cargo build --release

clean-dev:
	rm -rf build
	rm -rf include/protocols
	rm -rf .cache
	rm -f src/ipc.c

clean:
	rm -f $(DESTDIR)/usr/bin/ura
	rm -f $(DESTDIR)/usr/share/wayland-sessions/ura.desktop
	rm -rf $(DESTDIR)/etc/ura
	rm -rf $(DESTDIR)/usr/share/ura
	rm -f $(DESTDIR)/usr/share/zsh/site-functions/_ura*

clean-all: clean clean-dev

debug:
	$(MAKE) BUILD_TYPE=Debug

.PHONY: sudo-install install init build build-shell clean-dev clean clean-all debug

include/protocols:
	mkdir -p $@

include/protocols/%-protocol.h: ./protocols/%.xml
	wayland-scanner server-header $< $@

src/ipc.c: ./protocols/ura-ipc.xml
	wayland-scanner private-code $< $@

cmake/CPM.cmake:
	mkdir -p cmake
	wget -O $@ https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/get_cpm.cmake
