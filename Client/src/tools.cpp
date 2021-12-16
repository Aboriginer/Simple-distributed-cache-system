#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#include <iostream>
#include <sstream>

#include "tools.h"

std::string strRand(int length) {
  char tmp;
  std::string buffer;

  std::random_device rd;
  std::default_random_engine random(rd());

  for (int i = 0; i < length; i++) {
    tmp = random() % 36;
    if (tmp < 10) {
      tmp += '0';
    } else {
      tmp -= 10;
      tmp += 'A';
    }
    buffer += tmp;
  }
  return buffer;
}

void addfd(int epollfd, int fd, bool enable_et)
{
  struct epoll_event ev;
  ev.data.fd = fd;
  ev.events = EPOLLIN;
  if (enable_et)
    ev.events = EPOLLIN | EPOLLET;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

void split(const std::string& s,std::vector<std::string>& sv,const char flag) {
    sv.clear();
    std::istringstream iss(s);
    std::string temp;

    while (getline(iss, temp, flag)) {
        sv.push_back(temp);
    }
    return;
}

void parse_str_addr(const std::string str_addr, sockaddr_in& addr) {
  std::vector<std::string> ip_port;
	split(str_addr, ip_port, ':');
	addr.sin_addr.s_addr = inet_addr(ip_port[0].data());
	addr.sin_port = htons(atol(ip_port[1].data()));
}