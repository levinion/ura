#pragma once
#include <absl/container/flat_hash_map.h>
#include <sys/epoll.h>
#include <array>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <functional>
#include <memory>
#include <sys/timerfd.h>
#include <chrono>

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
    if (epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, &event) == -1)
      return;
    this->tasks[fd] = callback;
  }

  void remove_task(int fd) {
    if (!this->tasks.contains(fd))
      return;
    epoll_ctl(this->fd, EPOLL_CTL_DEL, fd, nullptr);
    this->tasks.erase(fd);
    close(fd);
  }

  template<typename Rep, typename Period>
  int set_timeout(
    std::function<void()> callback,
    std::chrono::duration<Rep, Period> timeout
  ) {
    auto fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (fd == -1)
      return -1;

    auto secs = std::chrono::duration_cast<std::chrono::seconds>(timeout);
    auto nsecs =
      std::chrono::duration_cast<std::chrono::nanoseconds>(timeout - secs);

    itimerspec timer_spec = {};
    timer_spec.it_value.tv_sec = secs.count();
    timer_spec.it_value.tv_nsec = nsecs.count();

    // flag 0 means relative time
    auto ret = timerfd_settime(fd, 0, &timer_spec, nullptr);
    if (ret == -1) {
      close(fd);
      return -1;
    }

    this->add_task(fd, [=, this]() {
      uint64_t expirations;
      while (read(fd, &expirations, sizeof(expirations)) > 0) {
        callback();
      }
      this->remove_task(fd);
      return true;
    });

    return fd;
  }

  void clear_timeout(int fd) {
    if (!this->is_task_active(fd))
      return;
    itimerspec clear = {};
    timerfd_settime(fd, 0, &clear, nullptr);
    this->remove_task(fd);
  }

  bool is_task_active(int fd) {
    return this->tasks.contains(fd);
  }

private:
  int fd;
  std::array<epoll_event, MAXEVENTS> events;
  absl::flat_hash_map<int, std::function<bool()>> tasks;
};

} // namespace ura
