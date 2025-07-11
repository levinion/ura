#include "ura/server.hpp"

int main() {
  auto server = ura::UraServer::init();
  server->run();
}
