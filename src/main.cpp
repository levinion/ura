#include "ura/server.hpp"

int main() {
  auto server = ura::UraServer::get_instance();
  server->init();
  server->run();
}
