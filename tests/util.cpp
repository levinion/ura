#include "ura/core/dispatcher.hpp"
#include "ura/core/status.hpp"
#include "ura/util/keybinding.hpp"
#include "ura/util/rgb.hpp"
#include "ura/util/string.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <unistd.h>

namespace {

bool test_string_helpers() {
  using namespace ura::util;
  const std::vector<std::string> expected { "", "alpha", "", "beta", "" };
  const auto parts = split("|alpha||beta|", '|');
  const std::vector<std::string> words { "alpha", "beta" };

  return ascii_lower("AbC-X") == "abc-x"
    && strip_ascii_whitespace(" \t hello\r\n") == "hello"
    && strip_ascii_whitespace(" \t\n").empty() && parts == expected
    && split("", ',') == std::vector<std::string> { "" }
  && join(words, ";") == "alpha;beta";
}

bool test_rgb_parser() {
  using ura::util::hex2rgba;
  const auto rgb = hex2rgba("#336699");
  const auto rgba = hex2rgba("#33669980");
  if (!rgb || !rgba)
    return false;

  constexpr float epsilon = 0.00001f;
  return std::abs((*rgb)[0] - 0x33 / 255.0f) < epsilon
    && std::abs((*rgb)[1] - 0x66 / 255.0f) < epsilon
    && std::abs((*rgb)[2] - 0x99 / 255.0f) < epsilon && (*rgb)[3] == 1.0f
    && std::abs((*rgba)[3] - 0x80 / 255.0f) < epsilon && !hex2rgba("336699")
    && !hex2rgba("#12345") && !hex2rgba("#gggggg");
}

bool test_keybinding_parser() {
  using namespace ura::util;
  const auto letter = get_keybinding_id(" super + shift + a ");
  const auto enter = get_keybinding_id("ctrl+Return");
  const auto wheel = get_keybinding_id("wheelup");

  return letter
    == construct_keybinding_id(
           WLR_MODIFIER_LOGO | WLR_MODIFIER_SHIFT,
           XKB_KEY_A
    )
    && enter == construct_keybinding_id(WLR_MODIFIER_CTRL, XKB_KEY_Return)
    && wheel
    == construct_keybinding_id(
         0,
         UraKeybindingDeviceType::Mouse
           | static_cast<uint32_t>(UraKeyCodeExtra::WheelUp)
    )
    && !get_keybinding_id("ctrl+not-a-key")
    && !get_keybinding_id("super+unknown-mod+a");
}

ura::StatusOr<int> value_or_error(bool fail) {
  if (fail)
    return ura::InvalidArgument("bad value");
  return 41;
}

ura::Status status_or_error(bool fail) {
  if (fail)
    return ura::NotFound("missing");
  return ura::Ok();
}

ura::StatusOr<int> add_one(bool fail) {
  int value = 0;
  ASSIGN_OR_RETURN(value, value_or_error(fail));
  return value + 1;
}

ura::Status propagate_status(bool fail) {
  RETURN_IF_ERROR(status_or_error(fail));
  return ura::Ok();
}

bool test_status_helpers() {
  const auto success = add_one(false);
  const auto failure = add_one(true);
  const auto propagated = propagate_status(true);
  return success && *success == 42 && !failure
    && failure.error().code == ura::ErrorCode::InvalidArgument
    && failure.error().message == "bad value" && !propagated
    && propagated.error().code == ura::ErrorCode::NotFound
    && ura::to_string(ura::ErrorCode::ScriptError)
    == std::string_view("ScriptError");
}

bool test_dispatcher_pipe() {
  auto created = ura::UraDispatcher<8>::init();
  if (!created)
    return false;
  auto dispatcher = std::move(created.value());

  int fds[2];
  if (pipe(fds) != 0)
    return false;

  int calls = 0;
  char received = 0;
  if (!dispatcher->add_task(fds[0], [&] {
        ++calls;
        return read(fds[0], &received, sizeof(received)) == 1;
      })) {
    close(fds[0]);
    close(fds[1]);
    return false;
  }

  const char sent = 'x';
  if (write(fds[1], &sent, sizeof(sent)) != 1) {
    dispatcher->remove_task(fds[0]);
    close(fds[1]);
    return false;
  }

  const auto result = dispatcher->dispatch();
  dispatcher->remove_task(fds[0]);
  close(fds[1]);
  return result && calls == 1 && received == sent;
}

bool test_dispatcher_timeout() {
  auto created = ura::UraDispatcher<8>::init();
  if (!created)
    return false;
  auto dispatcher = std::move(created.value());

  int calls = 0;
  const auto timer =
    dispatcher->set_timeout([&] { ++calls; }, std::chrono::milliseconds(5));
  if (timer == -1)
    return false;

  const auto result = dispatcher->dispatch();
  return result && calls == 1 && !dispatcher->is_task_active(timer);
}

} // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Expected one test case name\n";
    return 2;
  }

  const std::string_view name { argv[1] };
  bool passed = false;
  if (name == "strings")
    passed = test_string_helpers();
  else if (name == "rgb")
    passed = test_rgb_parser();
  else if (name == "keybinding")
    passed = test_keybinding_parser();
  else if (name == "status")
    passed = test_status_helpers();
  else if (name == "dispatcher-pipe")
    passed = test_dispatcher_pipe();
  else if (name == "dispatcher-timeout")
    passed = test_dispatcher_timeout();
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
