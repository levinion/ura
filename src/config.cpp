#define SOL_ALL_SAFETIES_ON 1

#include "ura/config.hpp"
#include <sol/sol.hpp>
#include "ura/ura.hpp"

namespace ura {

void UraConfigManager::load_config() {
  sol::state lua;
  lua.open_libraries(sol::lib::base, sol::lib::os);
  wlr_log(WLR_INFO, "Loading config");
  lua.script_file("/home/maruka/.config/ura/init.lua");
}

} // namespace ura
