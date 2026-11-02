#include "ura/core/entity.hpp"
#include <iostream>
#include <string_view>
#include <sol/sol.hpp>

namespace {

bool test_identity() {
  ura::UraEntityRegistry registry;
  auto* toplevel = reinterpret_cast<ura::UraToplevel*>(0x1000);
  auto toplevel_entity = registry.create_toplevel(toplevel);
  auto toplevel_id = ura::UraEntityRegistry::to_id(toplevel_entity);

  auto* output = reinterpret_cast<ura::UraOutput*>(0x2000);
  auto output_entity = registry.create_output(output);
  auto output_id = ura::UraEntityRegistry::to_id(output_entity);

  return toplevel_entity != entt::null && output_entity != entt::null
    && toplevel_id != 0 && output_id != 0 && toplevel_id != output_id
    && registry.contains(toplevel_entity) && registry.contains(toplevel_id)
    && registry.toplevel(toplevel_id) == toplevel
    && registry.toplevel(toplevel_entity) == toplevel
    && registry.output(output_id) == output
    && registry.output(output_entity) == output
    && registry.output(toplevel_id) == nullptr
    && registry.toplevel(output_id) == nullptr
    && registry.popup(toplevel_id) == nullptr
    && registry.layer_shell(output_id) == nullptr
    && ura::UraEntityRegistry::from_id(0) == entt::null
    && ura::UraEntityRegistry::from_id(1ull << 40) == entt::null;
}

bool test_stale_handle() {
  ura::UraEntityRegistry registry;
  auto* first = reinterpret_cast<ura::UraToplevel*>(0x3000);
  auto first_entity = registry.create_toplevel(first);
  auto first_id = ura::UraEntityRegistry::to_id(first_entity);

  registry.destroy(first_entity);
  auto invalidated = !registry.contains(first_entity)
    && !registry.contains(first_id) && registry.toplevel(first_id) == nullptr;

  auto* second = reinterpret_cast<ura::UraToplevel*>(0x4000);
  auto second_entity = registry.create_toplevel(second);
  auto second_id = ura::UraEntityRegistry::to_id(second_entity);

  return invalidated && second_id != first_id
    && registry.toplevel(second_id) == second
    && registry.toplevel(first_id) == nullptr;
}

bool test_userdata() {
  sol::state lua;
  lua.open_libraries(sol::lib::base);

  ura::UraEntityRegistry registry;
  auto* output = reinterpret_cast<ura::UraOutput*>(0x5000);
  auto entity = registry.create_output(output);
  auto id = ura::UraEntityRegistry::to_id(entity);

  auto table = lua.create_table();
  table["border_width"] = 4u;
  registry.set_userdata(id, table);

  auto border_width = registry.get_userdata<unsigned int>(id, "border_width");
  auto missing = registry.get_userdata<unsigned int>(id, "missing");
  auto stored = registry.userdata(id);

  if (!border_width || *border_width != 4 || missing || !stored.is<sol::table>())
    return false;

  registry.destroy(entity);
  auto destroyed = registry.userdata(id);
  return !registry.contains(id) && !destroyed.valid();
}

} // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Expected one test case name\n";
    return 2;
  }

  const std::string_view name { argv[1] };
  bool passed = false;
  if (name == "identity")
    passed = test_identity();
  else if (name == "stale-handle")
    passed = test_stale_handle();
  else if (name == "userdata")
    passed = test_userdata();
  else {
    std::cerr << "Unknown test case: " << name << '\n';
    return 2;
  }

  if (!passed) {
    std::cerr << "Test failed: " << name << '\n';
    return 1;
  }
  return 0;
}
