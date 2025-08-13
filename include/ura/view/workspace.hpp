#pragma once
#include "ura/view/toplevel.hpp"
#include <list>
#include <memory>
#include <optional>
#include <sol/table.hpp>

namespace ura {

class UraOutput;

class UraFocusStack {
public:
  std::optional<UraToplevel*> top();
  int size();
  std::optional<UraToplevel*> pop();
  void push(UraToplevel* toplevel);
  void remove(UraToplevel* toplevel);
  void move_to_top(UraToplevel* toplevel);
  bool is_top(UraToplevel* toplevel);
  bool contains(UraToplevel* toplevel);
  std::optional<UraToplevel*> find_active();
  void unfocus_all();

private:
  std::list<UraToplevel*> stack;
};

class UraWorkSpace {
public:
  std::list<UraToplevel*> toplevels;
  UraOutput* output;
  UraFocusStack focus_stack;
  static std::unique_ptr<UraWorkSpace> init();
  void enable();
  void disable();
  int index();
  std::optional<UraToplevel*> get_toplevel_at(int index);
  sol::table to_lua_table();
  void add(UraToplevel* toplevel);
  void remove(UraToplevel* toplevel);
};

} // namespace ura
