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
// Master IP地址
#define MASTER_IP "127.0.0.1"

// 缓冲区大小65535
#define BUF_SIZE 0xFFFF

// 服务器端口号
#define CLIENT_PORT 10000
#define CACHE_PORT 8000

const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int EPOLL_SIZE = 15;  // epoll支持的最大句柄数
const int MAX_THREADS_NUMBER = 10; // 最大线程数量
using namespace std;


class cMessage{
public:
    cMessage(uint32_t num, string key, uint32_t write){
        this->num = num;
        this->key = key;
        this->write = write;
    }
    cMessage(uint32_t addr, time_t timeFlag) {
        this->addr = addr;
        this->timeFlag = timeFlag;
    }
    uint32_t num = 0;
    string key = "1";
    uint32_t write = 1;
    uint32_t addr = 100;
    time_t timeFlag = 111;
    
};
class Master {

public:
    ConsistentHash cacheAddrHash;
    // 有参构造
    Master();
    // 启动服务器端
    void Start();
    void Close();
    //================================================================================

    class heartBeatResponse{
    public:
        heartBeatResponse(uint32_t addr, time_t timeFlag) {
            this->cacheServerAddr = addr;
            this->timeFlag = timeFlag;
        }
        uint32_t cacheServerAddr = 0;
        time_t timeFlag = 0;
    };

    uint32_t heartBeatInterval = 30 ;//???
    uint32_t a = 0;

    void handleMessage(cMessage* msg);

    uint32_t handleClientMessage(string msg);

    void handleHeartBeatResponse(int msg);
    bool heartBeatDetect(uint32_t cacheServerAddr);
    void periodicDetectCache();
    //================================================================================
private:
    void start_cache();
    void start_client();

    //================================================================================

    // static uint32_t getCacheServerAddr(string key);
    // static uint32_t getCacheServerAddr();

    bool furtherHeartBeatDetect(uint32_t cacheServerAddr);
    void updateKeyCacheMapByHeartBeat(uint32_t cacheAddr);


//    Master::heartBeatResponse* msg2heart(cMessage* msg);

    // static uint32_t loadBalance();
    void updateKeyCacheMapByCacheReq(string recv_buff_client);
    //================================================================================
//    vector<string> split(string s, string seperator);
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
    char status;              //  'p'/'r'
    int fd;
    int pair_fd;            //  pair fd
    std::string ip_port;    //  IP#PORT
    fdmap(int fd) :status('n'), fd(fd), pair_fd(-1), ip_port("0"){} //无参数的构造函数数组初始化时调用
};

#endif