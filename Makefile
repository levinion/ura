BUILD_TYPE ?= Release

PROTOCOL_FILES = $(wildcard protocols/*.xml)
PROTOCOL_HEADERS = $(patsubst protocols/%.xml,include/protocols/%-protocol.h,$(PROTOCOL_FILES))

install: build
	sudo install -Dm755 ./build/ura /usr/bin/
	sudo install -Dm644 ./assets/ura.desktop /usr/share/wayland-sessions/
	sudo install -d /etc/ura
	sudo install -Dm644 ./assets/init.lua /etc/ura/
	sudo rm -rf /usr/share/lua/5.1/ura
	sudo cp -r lua/ura /usr/share/lua/5.1

init: CMakeLists.txt include/protocols $(PROTOCOL_HEADERS)
	cmake -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: init
	cmake --build build -j$(shell nproc)

clean:
	rm -rf build
	rm -rf include/protocols
	sudo rm -rf /usr/bin/ura
	sudo rm -rf /usr/bin/uracil
	sudo rm -rf /usr/share/wayland-sessions/ura.desktop
	sudo rm -rf /usr/share/lua/5.1/ura/
	sudo rm -rf /etc/ura

debug:
	make BUILD_TYPE=Debug

.PHONY: default install init build clean debug

include/protocols:
	mkdir -p $@

include/protocols/%-protocol.h: ./protocols/%.xml
	wayland-scanner server-header $< $@
