#pragma once

#include "ura/toplevel.hpp"
#include <list>
#include <memory>

namespace ura {

class UraOutput;

class UraWorkSpace {
public:
  std::list<UraToplevel*> toplevels;
  UraOutput* output;
  static std::unique_ptr<UraWorkSpace> init();
  void enable(bool enabled);
  int index();
  void add(UraToplevel* toplevel);
};

} // namespace ura
