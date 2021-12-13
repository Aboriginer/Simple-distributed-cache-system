#ifndef RANDSTRING_HPP
#define RANDSTRING_HPP

#include <random>
#include <string>

// 生成随机string，由 0-9、A-Z 共 36 种字符组成
std::string strRand(int length);

void addfd(int epollfd, int fd, bool enable_et);

// 拆分string，flag：分隔符
void split(const std::string& s,std::vector<std::string>& sv,const char flag = '#');

// 解析addr string, eg. "127.0.0.1:8888"
void parse_str_addr(const std::string str_addr, sockaddr_in& addr);

#endif