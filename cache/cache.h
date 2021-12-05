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
//LRU链表
#include <LRU_cache.hpp>
//
#include <threadPool.hpp>
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
    // LRU底层结构
    static std::string message_to_client;
};

void addfd( int epollfd, int fd, bool enable_et );


//分割协议报文用的辅助函数：
std::vector<std::string> split(const std::string &s, const std::string &seperator){
  std::vector<std::string> result;
  typedef std::string::size_type string_size;
  string_size i = 0;
  
  while(i != s.size()){
    //找到字符串中首个不等于分隔符的字母；
    int flag = 0;
    while(i != s.size() && flag == 0){
      flag = 1;
      for(string_size x = 0; x < seperator.size(); ++x)
        if(s[i] == seperator[x]){
         ++i;
        flag = 0;
        break;
        }
    }
    
    //找到又一个分隔符，将两个分隔符之间的字符串取出；
    flag = 0;
    string_size j = i;
    while(j != s.size() && flag == 0){
      for(string_size x = 0; x < seperator.size(); ++x)
        if(s[j] == seperator[x]){
            flag = 1;
            break;
            }
      if(flag == 0) 
        ++j;
    }
    if(i != j){
      result.push_back(s.substr(i, j-i));
      i = j;
    }
  }
  return result;
}




#endif //CACHE_SOCK_CACHE_H
