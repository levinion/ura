install:
  just build
  sudo install -Dm755 ./build/ura /usr/bin/
  sudo install -Dm644 ./assets/ura.desktop /usr/share/wayland-sessions/

run:
  just build
  ./build/$(cat build/CMakeCache.txt | grep CMAKE_PROJECT_NAME | awk -F '=' '{print $2}')

debug:
  cmake -B build -DCMAKE_BUILD_TYPE=Debug
  just build
  gdb -q ./build/$(cat build/CMakeCache.txt | grep CMAKE_PROJECT_NAME | awk -F '=' '{print $2}')

init:
  mkdir -p include/protocols
  wayland-scanner server-header ./protocols/xdg-shell.xml include/protocols/xdg-shell-protocol.h
  wayland-scanner server-header ./protocols/wlr-layer-shell-unstable-v1.xml include/protocols/wlr-layer-shell-unstable-v1-protocol.h
  wayland-scanner server-header ./protocols/wlr-output-power-management-unstable-v1.xml include/protocols/wlr-output-power-management-unstable-v1-protocol.h
  cmake -B build

build:
  just init
  cmake --build build -j16

clean:
  rm -rf build
