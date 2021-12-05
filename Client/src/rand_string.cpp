#include <fcntl.h>
#include <sys/epoll.h>

#include <iostream>
#include <sstream>

#include "rand_string.hpp"

std::string strRand(int length) {                     // length: 产生字符串的长度
  char tmp;           // tmp: 暂存一个随机数
  std::string buffer; // buffer: 保存返回值

  std::random_device rd;                   // 产生一个 std::random_device 对象 rd
  std::default_random_engine random(rd()); // 用 rd 初始化一个随机数发生器 random

  for (int i = 0; i < length; i++) {
    tmp = random() % 36; // 随机一个小于 36 的整数，0-9、A-Z 共 36 种字符
    if (tmp < 10) { // 如果随机数小于 10，变换成一个阿拉伯数字的 ASCII
      tmp += '0';
    } else { // 否则，变换成一个大写字母的 ASCII
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
  // 设置socket为非阻塞模式
  // 套接字立刻返回，不管I/O是否完成，该函数所在的线程会继续运行
  //eg. 在recv(fd...)时，该函数立刻返回，在返回时，内核数据还没准备好会返回WSAEWOULDBLOCK错误代码
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
