#include "ura/ext_workspace.hpp"

#include "ura/core/server.hpp"
#include "ura/view/output.hpp"
#include "ura/view/toplevel.hpp"
#include "ura/view/view.hpp"

extern "C" {
#include <wlr/types/wlr_ext_workspace_v1.h>
}

#include <algorithm>
#include <utility>

namespace ura {

UraExtWorkspace::UraExtWorkspace(wl_display* display) {
  this->manager = wlr_ext_workspace_manager_v1_create(display, 1);
  if (!this->manager)
    return;

  this->commit_listener.notify = UraExtWorkspace::on_commit;
  wl_signal_add(&this->manager->events.commit, &this->commit_listener);
}

std::unique_ptr<UraExtWorkspace> UraExtWorkspace::create(wl_display* display) {
  auto manager = std::unique_ptr<UraExtWorkspace>(new UraExtWorkspace(display));
  if (!manager->manager)
    return {};
  return manager;
}

UraExtWorkspace::~UraExtWorkspace() {
  for (auto& group : this->groups) {
    for (auto& [_, workspace] : group->workspaces)
      wlr_ext_workspace_handle_v1_destroy(workspace);
    if (group->handle)
      wlr_ext_workspace_group_handle_v1_destroy(group->handle);
  }
  this->groups.clear();
  if (this->manager)
    wl_list_remove(&this->commit_listener.link);
}

UraExtWorkspace::Group* UraExtWorkspace::find_group(UraOutput* output) {
  for (auto& group : this->groups) {
    if (group->output == output)
      return group.get();
  }
  return nullptr;
}

UraExtWorkspace::Group*
UraExtWorkspace::find_group(wlr_ext_workspace_group_handle_v1* handle) {
  for (auto& group : this->groups) {
    if (group->handle == handle)
      return group.get();
  }
  return nullptr;
}

void UraExtWorkspace::create_workspace(Group& group, const Tag& tag) {
  auto id = group.output->name + ":" + tag.name();
  auto caps = EXT_WORKSPACE_HANDLE_V1_WORKSPACE_CAPABILITIES_ACTIVATE
    | EXT_WORKSPACE_HANDLE_V1_WORKSPACE_CAPABILITIES_DEACTIVATE;
  auto* workspace =
    wlr_ext_workspace_handle_v1_create(this->manager, id.c_str(), caps);
  if (!workspace)
    return;

  wlr_ext_workspace_handle_v1_set_name(workspace, tag.name().c_str());
  wlr_ext_workspace_handle_v1_set_group(workspace, group.handle);
  group.workspaces[tag] = workspace;
  group.tags[workspace] = tag;
}

void UraExtWorkspace::update_coordinates(Group& group) {
  for (std::size_t i = 0; i < this->tags.size(); ++i) {
    auto it = group.workspaces.find(this->tags[i]);
    if (it == group.workspaces.end())
      continue;
    auto coordinate = static_cast<uint32_t>(i);
    wlr_ext_workspace_handle_v1_set_coordinates(it->second, &coordinate, 1);
  }
}

void UraExtWorkspace::sync_group(Group& group) {
  if (!group.output)
    return;
  for (auto& [tag, workspace] : group.workspaces) {
    auto active =
      std::ranges::find(group.output->tags, tag) != group.output->tags.end();
    wlr_ext_workspace_handle_v1_set_active(workspace, active);
  }
}

void UraExtWorkspace::add_output(UraOutput* output) {
  if (!this->manager || !output || !output->output || this->find_group(output))
    return;

  auto group = std::make_unique<Group>();
  group->output = output;
  group->handle = wlr_ext_workspace_group_handle_v1_create(this->manager, 0);
  if (!group->handle)
    return;

  wlr_ext_workspace_group_handle_v1_output_enter(group->handle, output->output);
  auto* raw = group.get();
  this->groups.push_back(std::move(group));
  for (auto& tag : this->tags) this->create_workspace(*raw, tag);
  this->update_coordinates(*raw);
  this->sync_group(*raw);
}

void UraExtWorkspace::remove_output(UraOutput* output) {
  auto group = this->find_group(output);
  if (!group)
    return;

  for (auto& [_, workspace] : group->workspaces)
    wlr_ext_workspace_handle_v1_destroy(workspace);
  if (group->handle)
    wlr_ext_workspace_group_handle_v1_destroy(group->handle);

  std::erase_if(this->groups, [&](const auto& candidate) {
    return candidate.get() == group;
  });
}

void UraExtWorkspace::refresh() {
  std::set<Tag, TagLess> current;
  auto* server = UraServer::get_instance();
  if (server->view) {
    for (auto& [_, output] : server->view->outputs) {
      if (!output)
        continue;
      current.insert(output->tags.begin(), output->tags.end());
    }
    for (auto* toplevel : server->view->toplevels) {
      if (!toplevel)
        continue;
      auto& tags = toplevel->tags().values;
      current.insert(tags.begin(), tags.end());
    }
  }

  this->tag_set = current;
  this->tags.assign(current.begin(), current.end());

  for (auto& group : this->groups) {
    for (auto it = group->workspaces.begin(); it != group->workspaces.end();) {
      if (current.contains(it->first)) {
        ++it;
        continue;
      }
      wlr_ext_workspace_handle_v1_destroy(it->second);
      group->tags.erase(it->second);
      it = group->workspaces.erase(it);
    }
    for (auto& tag : this->tags) {
      if (!group->workspaces.contains(tag))
        this->create_workspace(*group, tag);
    }
    this->update_coordinates(*group);
    this->sync_group(*group);
  }
}

void UraExtWorkspace::sync_output(UraOutput* output) {
  auto group = this->find_group(output);
  if (group)
    this->sync_group(*group);
}

void UraExtWorkspace::sync_all() {
  for (auto& group : this->groups) this->sync_group(*group);
}

void UraExtWorkspace::on_commit(wl_listener* listener, void* data) {
  UraExtWorkspace* self = nullptr;
  self = wl_container_of(listener, self, commit_listener);
  self->handle_commit(static_cast<wlr_ext_workspace_v1_commit_event*>(data));
}

void UraExtWorkspace::handle_commit(wlr_ext_workspace_v1_commit_event* event) {
  if (!event || !event->requests)
    return;

  struct Desired {
    Group* group;
    std::set<Tag, TagLess> tags;
    bool initialized = false;
    bool has_activate = false;
  };

  std::vector<Desired> desired;

  auto find_desired = [&](Group* group) -> Desired& {
    for (auto& candidate : desired) {
      if (candidate.group == group)
        return candidate;
    }
    Desired next { .group = group, .tags = {} };
    desired.push_back(std::move(next));
    return desired.back();
  };

  wlr_ext_workspace_v1_request* request = nullptr;
  wl_list_for_each(request, event->requests, link) {
    if (
      request->type != WLR_EXT_WORKSPACE_V1_REQUEST_ACTIVATE
      && request->type != WLR_EXT_WORKSPACE_V1_REQUEST_DEACTIVATE
    )
      continue;

    auto* workspace = request->type == WLR_EXT_WORKSPACE_V1_REQUEST_ACTIVATE
      ? request->activate.workspace
      : request->deactivate.workspace;
    if (!workspace || !workspace->group)
      continue;
    auto* group = this->find_group(workspace->group);
    if (!group)
      continue;
    auto tag_it = group->tags.find(workspace);
    if (tag_it == group->tags.end())
      continue;

    auto& next = find_desired(group);
    if (request->type == WLR_EXT_WORKSPACE_V1_REQUEST_ACTIVATE) {
      if (!next.has_activate) {
        next.tags.clear();
        next.has_activate = true;
      }
      next.tags.insert(tag_it->second);
    } else {
      if (!next.has_activate && !next.initialized && group->output)
        next.tags.insert(
          group->output->tags.begin(),
          group->output->tags.end()
        );
      next.tags.erase(tag_it->second);
    }
    next.initialized = true;
  }

  for (auto& item : desired) {
    if (!item.group || !item.group->output)
      continue;
    std::vector<Tag> tags(item.tags.begin(), item.tags.end());
    item.group->output->set_tags(std::move(tags));
  }
}

} // namespace ura
