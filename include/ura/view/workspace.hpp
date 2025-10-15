#pragma once
#include "ura/util/vec.hpp"
#include "ura/view/toplevel.hpp"

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
  void move_to_bottom(UraToplevel* toplevel);
  bool is_top(UraToplevel* toplevel);
  bool contains(UraToplevel* toplevel);
  std::optional<UraToplevel*> find_active();
  void unfocus_all();

private:
  Vec<UraToplevel*> stack;
};

class UraWorkSpace {
public:
  Vec<UraToplevel*> toplevels;
  std::string output;
  UraFocusStack focus_stack;
  std::optional<std::string> name;
  static std::unique_ptr<UraWorkSpace> init();
  ~UraWorkSpace();
  static UraWorkSpace* from(uint64_t id);
  uint64_t id();
  void enable();
  void disable();
  int index();
  UraToplevel* get_toplevel_at(int index);
  sol::table to_lua_table();
  void add(UraToplevel* toplevel);
  void remove(UraToplevel* toplevel);
  void swap_toplevel(UraToplevel* src, UraToplevel* dst);
  void redraw();
};

} // namespace ura
