#include "ura/core/server.hpp"
#include "ura/core/state.hpp"
#include <CLI/CLI.hpp>
#include <print>
#include <string>
#include <utility>

int main(int argc, char** argv) {
  if (argc > 1 && !std::string(argv[1]).starts_with('-')) {
    argv = &argv[1];
    auto cmd = "ura-" + std::string(argv[0]);
    argv[0] = cmd.data();
    execvp(argv[0], argv);
    std::println("ura: command not found: {}", cmd);
    return 1;
  }
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
