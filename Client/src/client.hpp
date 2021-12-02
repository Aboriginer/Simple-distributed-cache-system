#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "lru.hpp"
#include "rand_string.hpp"

// 缓冲区大小65535
#define BUF_SIZE 0xFFFF

// Master IP地址
#define MASTER_IP "127.0.0.1"

// 服务器端口号
#define MASTER_PORT 8889
#define CACHESEVER_PORT 8887

#define KEY_LENGTH 3
#define VALUE_LENGTH 10

const int MAX_EVENT_NUMBER = 10000; //最大事件数

const int EPOLL_SIZE = 10;  // epoll支持的最大句柄数

class Client {

public:
    // 有参构造
    Client(int cache_size_local, char mode);

    // 启动客户端
    void Start();

    //向缓存系统读取key
    void Read();

    //以稳定的压力持续写入缓存系统
    void Write();

    //连接master
    void connect_master();

private:
    //client模式 -w write；-r read
    char mode_;

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd_;

    int master_sock;
    int cache_sever_sock;

    struct sockaddr_in master_addr;
    struct sockaddr_in cache_sever_addr;

    //本地Cache缓存条目大小
    int cache_size_local_;

    //key对应的value
    char value_[BUF_SIZE];

    char send_buff[BUF_SIZE];

    //Master返回的包含CacheSever IP的信息
    char recv_buff[BUF_SIZE];

    //本地Cache缓存
    cache::lru_cache<std::string, std::string>* cache_lru;
};

#endif