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
	sudo $(MAKE) build
	sudo $(MAKE) install

install:
	install -Dm755 ./build/ura $(DESTDIR)/usr/bin/ura
	install -Dm644 ./assets/ura.desktop $(DESTDIR)/usr/share/wayland-sessions/
	install -d $(DESTDIR)/etc/ura
	install -Dm644 ./assets/init.lua $(DESTDIR)/etc/ura/
	install -d $(DESTDIR)/usr/share/zsh/site-functions
	install -Dm644 ./assets/completions/zsh/* $(DESTDIR)/usr/share/zsh/site-functions/
	cp -r ura $(DESTDIR)/usr/share/
	chmod 755 $(DESTDIR)/usr/share/ura/bin/*

init: CMakeLists.txt include/protocols $(PROTOCOL_HEADERS)
	cmake -B build \
		$(NINJA) \
		$(LAUNCHER) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: init
	cmake --build build -j$(shell nproc)

clean-dev:
	rm -rf build
	rm -rf include/protocols

clean:
	rm $(DESTDIR)/usr/bin/ura
	rm $(DESTDIR)/usr/share/wayland-sessions/ura.desktop
	rm -rf $(DESTDIR)/etc/ura
	rm -rf $(DESTDIR)/usr/share/ura
	rm -rf $(DESTDIR)/usr/share/zsh/site-functions/_ura*

clean-all: clean clean-dev

debug:
	$(MAKE) BUILD_TYPE=Debug

.PHONY: sudo-install install init build clean-dev clean clean-all debug

include/protocols:
	mkdir -p $@

include/protocols/%-protocol.h: ./protocols/%.xml
	wayland-scanner server-header $< $@
