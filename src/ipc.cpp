#include "ura/core/ipc.hpp"
#include "ura/core/server.hpp"
#include "ura/core/lua.hpp"
#include <array>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <memory>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include "ura/core/log.hpp"
#include <nlohmann/json.hpp>

namespace ura {

std::string get_socket_path() {
  auto runtime_dir = getenv("XDG_RUNTIME_DIR");
  assert(runtime_dir);
  auto dir = std::filesystem::path(runtime_dir);
  for (int i = 0;; i++) {
    auto socket_path = dir / std::format("ura-socket-{}", i);
    if (std::filesystem::exists(socket_path))
      continue;
    setenv("URA_SOCKET_PATH", socket_path.c_str(), true);
    return socket_path;
  }
}

std::unique_ptr<UraIPC> UraIPC::init() {
  int ret;
  auto ipc = std::make_unique<UraIPC>();
  auto fd = socket(AF_UNIX, SOCK_DGRAM, 0);
  assert(fd != -1);
  ipc->fd = fd;

  ipc->socket_path = get_socket_path();

  auto flags = fcntl(fd, F_GETFL, 0);
  assert(flags != -1);
  ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  assert(ret != -1);

  sockaddr_un server_addr {};
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sun_family = AF_UNIX;
  strncpy(
    server_addr.sun_path,
    ipc->socket_path.c_str(),
    sizeof(server_addr.sun_path) - 1
  );

  unlink(ipc->socket_path.c_str());
  ret = bind(fd, (sockaddr*)&server_addr, sizeof(server_addr));
  assert(ret != -1);
  return ipc;
}

UraIPC::~UraIPC() {
  if (this->fd != -1)
    close(fd);
  unlink(this->socket_path.c_str());
}

std::optional<UraIPCRequestMessage> UraIPC::try_read() {
  memset(&this->client_addr, 0, sizeof(this->client_addr));
  this->client_len = sizeof(this->client_addr);
  auto len = recvfrom(
    this->fd,
    &this->buf,
    this->buf.size(),
    0,
    (sockaddr*)&this->client_addr,
    &client_len
  );
  if (len == -1) {
    return {};
  }
  auto message = std::string(buf.begin(), buf.begin() + len);
  auto msg = UraIPCRequestMessage::from_str(message);
  if (msg)
    return msg.value();
  else
    log::error("IPC ERROR");
  return {};
}

void UraIPC::try_send(UraIPCReplyMessage& message) {
  auto msg = message.to_str();
  auto status = sendto(
    this->fd,
    msg.data(),
    msg.size(),
    0,
    (struct sockaddr*)&this->client_addr,
    this->client_len
  );
  if (status == -1) {
    perror("ipc: send failed");
  }
}

void UraIPC::try_handle() {
  auto request = this->try_read();
  if (!request)
    return;
  auto code = request->body;
  auto server = UraServer::get_instance();
  auto reply = UraIPCReplyMessage {};
  auto result = server->lua->execute(code);
  if (result) {
    reply.status = "success";
    reply.body = result.value();
  } else {
    reply.status = "fail";
    reply.body = result.error();
  }
  this->try_send(reply);
}

std::optional<UraIPCRequestMessage>
UraIPCRequestMessage::from_str(std::string_view str) {
  auto j = nlohmann::json::parse(str);
  auto message = UraIPCRequestMessage {};
  if (j.contains("method") && j["method"].is_string())
    message.method = j["method"];
  else
    return {};
  if (j.contains("body") && j["body"].is_string())
    message.body = j["body"];
  else
    return {};
  return message;
}

std::string UraIPCReplyMessage::to_str() {
  auto j = nlohmann::json {};
  j["status"] = this->status;
  j["body"] = this->body;
  return j.dump(-1, ' ', false, nlohmann::detail::error_handler_t::ignore);
}

} // namespace ura
