#ifndef RANDSTRING_HPP
#define RANDSTRING_HPP

#include <random>
#include <string>

std::string strRand(int length);

void addfd(int epollfd, int fd, bool enable_et);

#endif