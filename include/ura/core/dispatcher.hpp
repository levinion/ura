#pragma once
#include <sys/epoll.h>
#include <array>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <functional>
#include <memory>
#include <unordered_map>
#include <sys/timerfd.h>

namespace ura {

template<int MAXEVENTS>
class UraDispatcher {
public:
  static std::unique_ptr<UraDispatcher<MAXEVENTS>> init() {
    auto dispatcher = std::make_unique<UraDispatcher<MAXEVENTS>>();
    dispatcher->fd = epoll_create1(0);
    assert(dispatcher->fd != -1);
    return dispatcher;
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
    [[maybe_unused]] auto ret = epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, &event);
    assert(ret != -1);
    this->tasks[fd] = callback;
  }

  void remove_task(int fd) {
    assert(this->tasks.contains(fd));
    [[maybe_unused]] auto ret = epoll_ctl(this->fd, EPOLL_CTL_DEL, fd, nullptr);
    assert(ret != -1);
    this->tasks.erase(fd);
  }

  void schedule_task(std::function<bool()> callback, uint64_t millisecond) {
    auto fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (fd == -1)
      return;
    itimerspec timer_spec;
    // ms to s and ns
    timer_spec.it_value.tv_sec = millisecond / 1000;
    timer_spec.it_value.tv_nsec = (millisecond % 1000) * 1000000;
    // interval: only exec the task once
    timer_spec.it_interval.tv_sec = 0;
    timer_spec.it_interval.tv_nsec = 0;
    // flag 0 means relative time
    [[maybe_unused]] auto ret = timerfd_settime(fd, 0, &timer_spec, nullptr);
    assert(ret != -1);

    this->add_task(fd, [=, this]() {
      // consume the event
      uint64_t expirations;
      [[maybe_unused]] auto n = read(fd, &expirations, sizeof(expirations));
      assert(n == sizeof(expirations));
      bool success = callback();
      this->remove_task(fd);
      close(fd);
      return success;
    });
  }

private:
  int fd;
  std::array<epoll_event, MAXEVENTS> events;
  std::unordered_map<int, std::function<bool()>> tasks;
};

} // namespace ura
