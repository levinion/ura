#pragma once
#include <sys/epoll.h>
#include <array>
#include <cassert>
#include <functional>
#include <unordered_map>
#include <vector>
#include "ura/ura.hpp"

namespace ura {

template<int MAXEVENTS>
class UraDispatcher {
public:
  void init(wl_event_loop* event_loop) {
    this->fd = epoll_create1(0);
    assert(this->fd != -1);
    this->event_loop = event_loop;
  }

  // if it goes wrong, then a false value will be returned
  bool dispatch() {
    int nfds = epoll_wait(this->fd, this->events.data(), MAXEVENTS, -1);
    if (nfds == -1) {
      if (errno == EINTR)
        return true;
      return false;
    }
    for (int i = 0; i < nfds; i++) {
      auto current_fd = this->events[i].data.fd;
      if (this->tasks.contains(current_fd)) {
        auto success = this->tasks[current_fd]();
        if (!success)
          return false;
      }
    }
    return true;
  }

  void add_task(int fd, std::function<bool()> callback) {
    epoll_event event {};
    event.events = EPOLLIN;
    event.data.fd = fd;
    auto ret = epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, &event);
    assert(ret != -1);
    this->tasks[fd] = callback;
  }

private:
  int fd;
  wl_event_loop* event_loop;
  std::array<epoll_event, MAXEVENTS> events;
  std::unordered_map<int, std::function<bool()>> tasks;
};

} // namespace ura
