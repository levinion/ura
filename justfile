run:
  just build
  ./build/$(cat build/CMakeCache.txt | grep CMAKE_PROJECT_NAME | awk -F '=' '{print $2}')

debug:
  cmake -B build -DCMAKE_BUILD_TYPE=Debug
  just build
  gdb -q ./build/$(cat build/CMakeCache.txt | grep CMAKE_PROJECT_NAME | awk -F '=' '{print $2}')

init:
  mkdir -p protocol
  wayland-scanner server-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml protocol/xdg-shell-protocol.h
  cmake -B build

build:
  just init
  cmake --build build

clean:
  rm -rf build

install:
  just build
  sudo install -Dm755 ./build/ura /usr/bin/
  sudo install -Dm644 ./assets/ura.desktop /usr/share/wayland-sessions/
