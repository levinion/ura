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
	install -d $(DESTDIR) /usr/bin
	install -Dm755 ./build/ura $(DESTDIR)/usr/bin/
	install -d $(DESTDIR) /usr/share/wayland-sessions
	install -Dm644 ./assets/ura.desktop $(DESTDIR)/usr/share/wayland-sessions/
	install -d $(DESTDIR)/etc/ura
	install -Dm644 ./assets/init.lua $(DESTDIR)/etc/ura/
	install -d $(DESTDIR)/usr/share/zsh/site-functions
	install -Dm644 ./assets/completions/zsh/* $(DESTDIR)/usr/share/zsh/site-functions/
	cp -r ura $(DESTDIR)/usr/share/
	install -Dm755 ./scripts/* $(DESTDIR)/usr/bin/
	install -Dm755 ./shell/target/release/ura-shell $(DESTDIR)/usr/bin/

init: CMakeLists.txt include/protocols $(PROTOCOL_HEADERS) src/ipc.c
	cmake -B build \
		$(NINJA) \
		$(LAUNCHER) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: init
	cmake --build build -j$(shell nproc)
	$(MAKE) build-shell

build-shell:
	cd shell && cargo build --release

clean-dev:
	rm -rf build
	rm -rf include/protocols

clean:
	rm -f $(DESTDIR)/usr/bin/ura
	rm -f $(DESTDIR)/usr/share/wayland-sessions/ura.desktop
	rm -rf $(DESTDIR)/etc/ura
	rm -rf $(DESTDIR)/usr/share/ura
	rm -f $(DESTDIR)/usr/share/zsh/site-functions/_ura*

clean-all: clean clean-dev

debug:
	$(MAKE) BUILD_TYPE=Debug

.PHONY: sudo-install install init build clean-dev clean clean-all debug

include/protocols:
	mkdir -p $@

include/protocols/%-protocol.h: ./protocols/%.xml
	wayland-scanner server-header $< $@

src/ipc.c: ./protocols/ura-ipc.xml
	wayland-scanner private-code $< $@

