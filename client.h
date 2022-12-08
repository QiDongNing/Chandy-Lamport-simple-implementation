#ifndef CLIENT_H_
#define CLIENT_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <utility>
#include <vector>
#include <string>

class Client {
 public:
  Client(std::string host, int port);
  ~Client();

  static int Socket(int domain, int type, int protocol);
  static void Connect(int sock_fd, struct sockaddr* addr, socklen_t len);

  int conn_fd_;
  struct sockaddr_in addr_;
  socklen_t addr_size_;
};

#endif  // CLIENT_H_