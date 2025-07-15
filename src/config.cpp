#define SOL_ALL_SAFETIES_ON 1

#include <filesystem>
#include "ura/server.hpp"
#include "ura/config.hpp"
#include <sol/sol.hpp>
#include <string>

namespace ura {
std::unique_ptr<UraConfig> UraConfig::init() {
  return std::make_unique<UraConfig>();
}

void UraConfig::load() {
  auto server = UraServer::get_instance();
  std::string _home_dir = getenv("HOME");
  auto home_dir = std::filesystem::path(_home_dir);
  auto config_path = home_dir / ".config/ura/init.lua";
  server->lua->execute_file(config_path);
}

} // namespace ura
