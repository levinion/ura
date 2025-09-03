#include "ura/core/server.hpp"
#include <CLI/CLI.hpp>
#include <print>

#define URA_PROJECT_VERSION "0.1.4"

void run() {
  auto server = ura::UraServer::get_instance();
  server->init();
  server->run();
}

void print_version() {
  std::println("Ura {}", URA_PROJECT_VERSION);
}

int main(int argc, char** argv) {
  bool version = false;
  auto cli =
    CLI::App { "A highly customizable Wayland compositor driven by Lua ",
               "ura" };
  cli.add_flag("-v,--version", version, "Show version information");
  CLI11_PARSE(cli, argc, argv);

  if (version) {
    print_version();
    return 0;
  }

  run();
}
