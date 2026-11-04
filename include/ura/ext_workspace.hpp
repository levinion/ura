#pragma once

#include <memory>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include <wayland-server-core.h>

#include "ura/util/tag.hpp"

struct wl_display;
struct wlr_ext_workspace_manager_v1;
struct wlr_ext_workspace_group_handle_v1;
struct wlr_ext_workspace_handle_v1;
struct wlr_ext_workspace_v1_commit_event;

namespace ura {

class UraOutput;

// Adapts Ura's tag model to ext-workspace-v1. Tags remain the source of truth
// for window visibility; this class only publishes them as workspaces.
class UraExtWorkspace {
public:
  static std::unique_ptr<UraExtWorkspace> create(wl_display* display);
  ~UraExtWorkspace();

  void add_output(UraOutput* output);
  void remove_output(UraOutput* output);
  // Reconcile the published workspaces with the tags currently present on
  // outputs and windows. Active output tags are part of that set.
  void refresh();
  void sync_output(UraOutput* output);
  void sync_all();

private:
  struct Group {
    UraOutput* output = nullptr;
    wlr_ext_workspace_group_handle_v1* handle = nullptr;
    std::map<Tag, wlr_ext_workspace_handle_v1*, TagLess> workspaces;
    std::unordered_map<wlr_ext_workspace_handle_v1*, Tag> tags;
  };

  explicit UraExtWorkspace(wl_display* display);
  static void on_commit(wl_listener* listener, void* data);
  Group* find_group(UraOutput* output);
  Group* find_group(wlr_ext_workspace_group_handle_v1* handle);
  void create_workspace(Group& group, const Tag& tag);
  void update_coordinates(Group& group);
  void sync_group(Group& group);
  void handle_commit(wlr_ext_workspace_v1_commit_event* event);

  wlr_ext_workspace_manager_v1* manager = nullptr;
  wl_listener commit_listener {};
  std::vector<std::unique_ptr<Group>> groups;
  std::set<Tag, TagLess> tag_set;
  std::vector<Tag> tags;
};

} // namespace ura
