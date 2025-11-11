BUILD_TYPE ?= Release

PROTOCOL_FILES = $(wildcard protocols/*.xml)
PROTOCOL_HEADERS = $(patsubst protocols/%.xml,include/protocols/%-protocol.h,$(PROTOCOL_FILES))

install: build
	sudo install -Dm755 ./build/ura /usr/bin/ura
	sudo install -Dm644 ./assets/ura.desktop /usr/share/wayland-sessions/
	sudo install -d /etc/ura
	sudo install -Dm644 ./assets/init.lua /etc/ura/
	sudo install -d /usr/share/zsh/site-functions
	sudo install -Dm644 ./assets/completions/zsh/* /usr/share/zsh/site-functions/
	${MAKE} lib

init: CMakeLists.txt include/protocols $(PROTOCOL_HEADERS)
	cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: init
	cmake --build build -j$(shell nproc)

clean:
	rm -rf build
	rm -rf include/protocols

clean-all: clean
	sudo rm -rf /usr/share/wayland-sessions/ura.desktop
	sudo rm -rf /etc/ura
	sudo rm -rf /usr/share/ura

debug:
	make BUILD_TYPE=Debug

lib:
	sudo rm -rf /usr/share/ura
	sudo cp -r ura /usr/share/

.PHONY: default install init build clean clean-all debug rsync lib

include/protocols:
	mkdir -p $@

include/protocols/%-protocol.h: ./protocols/%.xml
	wayland-scanner server-header $< $@
