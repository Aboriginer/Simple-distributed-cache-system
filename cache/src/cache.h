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
#include <vector>
#include <atomic>
//LRU链表
#include "LRU_cache.hpp"
//
#include "threadPool.hpp"
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

const int MAX_THREADS_NUMBER = 10; // 最大线程数量

const int LRU_CAPACITY = 10;

class Cache {

public:
    Cache(int cache_size_local, std::string status_, std::string local_cache_IP, std::string port_for_client, std::string port_for_cache);

    // 启动Cache
    void Start();


    // TODO：接收master指挥的扩缩容信息

private:
    int cache_size_local_;
    std::string replicaIP;
    int replicaPort;
    std::atomic<int> status;                //扩缩容用到的状态
    std::atomic<std::string> targetCacheIP;     //扩缩容时用到的另一个cache的IP
    std::atomic<std::string> targetCachePort;    //扩缩容用到的另一个cache的端口
    std::atomic<std::string> bufferReplica;
    // 向master发送心跳包
    static void Heartbeat();
    // 接收master信息
    static void Master_chat();
    // 与client通信
    static void Client_chat();
    // 与cache server通信
    static void CachePass();
    // LRU底层结构
    static std::string message_to_client;
    // IP port信息
    static std::string status_, local_cache_IP_, port_for_client_, port_for_cache_;

};

void addfd( int epollfd, int fd, bool enable_et );


//分割协议报文用的辅助函数：


#endif //CACHE_SOCK_CACHE_H