#ifndef MASTER_H
#define MASTER_H

#include <arpa/inet.h> 
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <thread>
#include <vector>
// #include "cMessage.hpp"
#include <unordered_map>
#include <stack>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <functional>
#include <sstream>
#include "ConsistentHash.hpp"
#include <unistd.h>
// Master IP地址
#define MASTER_IP "127.0.0.1"

// 缓冲区大小65535
#define BUF_SIZE 0xFFFF

// 服务器端口号
#define CLIENT_PORT 10000
// #define CACHE_PORT 
#define CACHE_PORT 8889

const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int EPOLL_SIZE = 20;  // epoll支持的最大句柄数
const int MAX_THREADS_NUMBER = 10; // 最大线程数量
using namespace std;


class Master {

public:
    // 一致性哈希类定义
    ConsistentHash cacheAddrHash;
    // cache是否存活的map
    unordered_map<string, time_t> cacheAddrMap;
    // 接入的cache列表：fd-cache详细信息
    std::unordered_map<int, struct fdmap *> clients_list;
    // 接入的cache对应的fd列表：
    std::vector<int> fd_node;
    // 主cache？这的备注没有粘过来 T T
    stack<int> pcache;
    // 备份cache？
    stack<int> rcache;
    int epoll_events_count;
    // 参构造函数
    Master();
    // 启动服务器端
    void Start();
    void Close();
    // 心跳存活的时间间隔
    uint32_t heartBeatInterval = 5 ;//???

private:
    // cache接入
    void start_cache();
    // client接入
    void start_client();
    // 周期性地检测cache是否存活
    void periodicDetectCache();
    // 缩容函数
    void shrinkageCapacity();
    // 处理client信息（读写请求响应）
    string handleClientMessage(string msg);
    // 处理cache心跳信息（更新时间戳）
    void handleHeartBeatResponse(string msg);
    // 判断一个cache是否存活
    bool heartBeatDetect(int fd);

};

// / 注册新的fd到epollfd中
// 参数enable_et表示是否启用ET模式，如果为True则启用，否则使用LT模式
static void addfd( int epollfd, int fd, bool enable_et )
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if( enable_et )
        ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    // 设置socket为nonblocking模式
    // 执行完就转向下一条指令，不管函数有没有返回。
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0)| O_NONBLOCK);
    printf("fd added to epoll!\n\n");
} 


static vector<string> split(string s, string seperator){
    vector<string> result;

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

struct fdmap {
    char status;            //  'P'/'R'
    int fd;
    int pair_fd;            //  pair fd
    std::string ip_port;    //  IP#PORT
    fdmap(int fd) :status('n'), fd(fd), pair_fd(-1), ip_port("0"){} //无参数的构造函数数组初始化时调用
};

#endif