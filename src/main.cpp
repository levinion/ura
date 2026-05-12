#include "ura/core/server.hpp"
#include <print>
#include <string>
#include <cxxopts.hpp>

int main(int argc, char** argv) {
  if (argc > 1 && !std::string(argv[1]).starts_with('-')) {
    argv = &argv[1];
    auto cmd = "ura-" + std::string(argv[0]);
    argv[0] = cmd.data();
    execvp(argv[0], argv);
    std::println("ura: command not found: {}", cmd);
    return 1;
  }

  auto options = cxxopts::Options(
    "ura",
    "A highly customizable Wayland compositor driven by Lua"
  );

  options.add_options()(
    "v,version",
    "Show version information",
    cxxopts::value<bool>()->default_value("false")
  )("h,help","Print help message");

  auto result = options.parse(argc, argv);

  if (result["version"].as<bool>()) {
    std::println("Ura {}", URA_PROJECT_VERSION);
    return 0;
  }

  if (result.contains("help")) {
    std::println("{}", options.help());
  }

  auto server = ura::UraServer::get_instance();
  server->init();
  server->run();
}
