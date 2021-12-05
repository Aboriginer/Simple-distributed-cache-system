//
// Created by Aboriginer on 12/1/2021.
//

#ifndef CACHE_SOCK_CACHE_H
#define CACHE_SOCK_CACHE_H

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <thread>

// 缓冲区大小65535
#define BUF_SIZE 0xFFFF

// Master IP地址
#define MASTER_IP "127.0.0.1"
#define CACHE_SERVER_IP "127.0.0.1"
// 服务器端口号
#define MASTER_PORT 8889
#define SERV_CLIENT_CACHE_SEVER_PORT 8887

#define KEY_LENGTH 3
#define VALUE_LENGTH 10

const int MAX_EVENT_NUMBER = 10000; //最大事件数

const int EPOLL_SIZE = 10;  // epoll支持的最大句柄数

class Cache {

public:
    Cache(int cache_size_local);

    // 启动Cache
    void Start();


    // TODO：接收master指挥的扩缩容信息

private:
    int cache_size_local_;
    // 向master发送心跳包
    static void Heartbeat();
    // 与client通信
    static void Client_chat();
    // 与cache server通信
    static void Cache_server_chat();

    // TODO:LRU
};

void addfd( int epollfd, int fd, bool enable_et );

#endif //CACHE_SOCK_CACHE_H
