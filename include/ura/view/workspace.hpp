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

class UraWorkspace {
public:
  Vec<UraToplevel*> toplevels;
  std::string output_name;
  UraFocusStack focus_stack;
  std::optional<std::string> name;
  double scale = 1.0;
  static std::unique_ptr<UraWorkspace> init();
  ~UraWorkspace();
  static UraWorkspace* from(uint64_t id);
  uint64_t id();
  void enable();
  void disable();
  int index();
  UraToplevel* get_toplevel_at(int index);
  void add(UraToplevel* toplevel);
  void remove(UraToplevel* toplevel);
  void swap_toplevel(UraToplevel* src, UraToplevel* dst);
  void set_scale(double scale);
  UraOutput* output();
  std::optional<Vec4<int>> geometry();
};

} // namespace ura
