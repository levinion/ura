#pragma once

#include <sys/un.h>
#include <unistd.h>
#include <array>
#include <memory>
#include <optional>

namespace ura {

class UraIPCRequestMessage {
public:
  std::string method;
  std::string body;
  static std::optional<UraIPCRequestMessage> from_str(std::string_view str);
};

class UraIPCReplyMessage {
public:
  std::string status;
  std::string body;
  std::string to_str();
};

class UraIPC {
public:
  int fd;
  static std::unique_ptr<UraIPC> init();
  ~UraIPC();
  void try_handle();

private:
  std::optional<UraIPCRequestMessage> try_read();
  void try_send(UraIPCReplyMessage& message);
  std::array<char, 4096> buf;
  sockaddr_un client_addr;
  socklen_t client_len;
  std::string socket_path;
};

} // namespace ura
