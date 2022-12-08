#include "client.h"
#include "utility.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <random>
#include <thread>

Client::Client(std::string host, int port) {
  struct hostent *he;
  if ((he = gethostbyname(host.c_str())) == NULL) {
    perror("gethostbyname");
    exit(1);
  }

  memset(&this->addr_, 0, sizeof(this->addr_));
  memcpy(&this->addr_.sin_addr, he->h_addr_list[0], he->h_length);

  this->addr_.sin_family = AF_INET;
  this->addr_.sin_port = htons(port);
  this->addr_size_ = sizeof(addr_);

  //printf("\nClient: connecting to %s:%d\n", host.c_str(), port); }
}

Client::~Client() {
  //shutdown(this->conn_fd_, SHUT_WR);
  //close(this->conn_fd_);
}

int Client::Socket(int domain, int type, int protocol) {
  int sock_fd;
  if ((sock_fd = socket(domain, type, protocol)) == -1) {
    perror("socket");
    exit(1);
  }
  return sock_fd;
}

void Client::Connect(int sock_fd, struct sockaddr *addr, socklen_t len) {
  if (connect(sock_fd, addr, len) == -1) {
    perror("connect");
    exit(1);
  }
}
