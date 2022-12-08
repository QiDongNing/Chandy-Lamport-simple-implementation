#include "server.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <thread>

Server::Server(int port) {
  memset(&this->addr_, 0, sizeof(addr_));
  this->addr_.sin_family = AF_INET;
  this->addr_.sin_addr.s_addr = inet_addr("127.0.0.1");
  this->addr_.sin_port = htons(port);
  this->addr_size_ = sizeof(addr_);

  this->listener_fd_ = Socket(AF_INET, SOCK_STREAM, 0);
  Bind(this->listener_fd_, (struct sockaddr*)&this->addr_, this->addr_size_);
  Listen(this->listener_fd_, 5);

  char server_addr[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &this->addr_.sin_addr, server_addr, sizeof(server_addr));//将“二进制整数” －> “点分十进制”
  printf("Server:  Listening on %s:%d\n", server_addr, port);
}

Server::~Server() {
  //close(this->listener_fd_);
}


int Server::Socket(int domain, int type, int protocol) {
  int sock_fd;
  if ((sock_fd = socket(domain, type, protocol)) == -1) {
    perror("socket");
    exit(1);
  }
  return sock_fd;
}

void Server::Bind(int sock_fd, const sockaddr* addr, socklen_t len) {
  int yes = 1;
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }
  if (bind(sock_fd, addr, len) == -1) {
    perror("bind");
    exit(1);
  }
}

void Server::Listen(int sock_fd, int backlog_size) {
  if (listen(sock_fd, backlog_size) == -1) {
    perror("listen");
    exit(1);
  }
}

int Server::Accept(int sock_fd, struct sockaddr* addr, socklen_t* len) {
  int conn_fd;
  if ((conn_fd = accept(sock_fd, addr, len)) == -1) {
    perror("accept");
    exit(1);
  }
  return conn_fd;
}
