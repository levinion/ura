#include "ura/core/server.hpp"
#include "ura/core/state.hpp"
#include <CLI/CLI.hpp>
#include <print>
#include <string>

int main(int argc, char** argv) {
  bool version = false;
  std::optional<std::string> config_path;

  auto cli =
    CLI::App { "A highly customizable Wayland compositor driven by Lua ",
               "ura" };
  cli.add_flag("-v,--version", version, "Show version information");
  cli.add_option("-c,--config", config_path, "Set configuration file path")
    ->check(CLI::ExistingFile);
  CLI11_PARSE(cli, argc, argv);

  if (version) {
    std::println("Ura {}", URA_PROJECT_VERSION);
    return 0;
  }

  auto state = ura::UraState::init(std::move(config_path));
  auto server = ura::UraServer::get_instance();
  server->init(std::move(state));
  server->run();
}
