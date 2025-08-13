#pragma once

#include <sys/un.h>
#include <unistd.h>
#include <array>
#include <expected>
#include <memory>
#include <optional>

namespace ura {

#define URA_SOCKET_PATH "/tmp/ura-socket"

class UraIPCRequestMessage {
public:
  std::string method;
  std::string body;
  static std::expected<UraIPCRequestMessage, std::string>
  from_str(std::string& str);
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
};

} // namespace ura
