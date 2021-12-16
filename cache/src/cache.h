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
#include <unordered_map>
// #include <atomic>
#include <functional>
#include <mutex>
#include <algorithm>
//LRU链表
#include "LRU_cache.hpp"
#include "threadPool.hpp"
#include "Timer.hpp"
#include "ConsistentHash.hpp"
// 缓冲区大小65535
#define BUF_SIZE 0xFFFF

// Master IP地址
#define MASTER_IP "127.0.0.1"
#define CACHE_SERVER_IP "127.0.0.1"
// 服务器端口号
#define MASTER_PORT 8889
#define MASTER_PORT_2 8890
#define SERV_CLIENT_CACHE_SEVER_PORT 8887

#define KEY_LENGTH 3
#define VALUE_LENGTH 10
#define NO_INIT 0
#define SUCCESS_INIT 1
#define ERROR_INIT 2

const int MAX_EVENT_NUMBER = 10000; //最大事件数

const int EPOLL_SIZE = 10;  // epoll支持的最大句柄数

const int MAX_THREADS_NUMBER = 10; // 最大线程数量

const int LRU_CAPACITY = 10;



class Cache {

public:
    Cache(int cache_size_local, std::string status_, std::string local_cache_IP, std::string port_for_client, std::string port_for_cache);
    // 启动Cache
    void Start();

private:
    ConsistentHash hash_maker;
    std::atomic<int> is_initialed;
    LRU_Cache<std::string, std::string> MainCache;
    int cache_size_local_;
    std::mutex status_mutex;
    //用于管理缓冲区的锁
    std::mutex kv_mutex;
    //用于管理退出进程
    std::mutex end_mutex;
    std::string replica_IP_, port_for_replica;  // TODO：这两行好像不需要了？在69行写了target_IP_, target_port_
    std::string primary_IP_, port_for_primary;
    //向备份发送的信息
    std::string kv_to_replica; // 用于存储向replica cache传递的key key#value
    bool kv_update_flag;    // true: key key#value有更新
    // cache_list用于存储所有cache的IP和port，包括本地cache的IP和port，TODO:我觉得扩缩容用的other_Cache可以直接用cache_list代替
    // std::unordered_map<std::string ,std::string> cache_list;
    std::vector<pair<std::string , std::string>> cache_list;
    bool cache_list_update_flag;    // true：cache_list有更新，整个cache_list发送给replica cache(以下四种情况为true: 1.cache_list初始化2.cache扩容3.cache缩容4.cache宕机)
    std::string target_IP_, target_port_; // primary状态：target_IP_为备份cache IP, replica状态：target_IP_为主cache IP
    // 扩缩容使用
    std::string dying_cache_IP_, dying_cache_Port;
    std::vector<std::string> otherIP, otherPort, out_key;
    //其他cache的地址和port,左值为IP，右值为port
    std::vector<pair<std::string , std::string>> other_Cache;
    // 从master接受初始化信息
    void initial(int cache_master_sock, char recv_buff_initial[BUF_SIZE]);
    // 向master发送心跳包
    void Heartbeat();
    // 与client通信
    void Client_chat();
    // 容灾通信
    void replica_chat();
    // 与cache server通信
    void cache_pass();
    //向单个IP地址传递信息
    void to_single_cache(std::string &ip, std::string &port, std::string &key);
    //从单个IP地址接受信息
    void from_single_cache(std::string &ip, std::string &port);
    //从master存取信息
    void ReadFromMaster(std::string message);
    // IP port信息
    void update_cache(std::string &IP, std::string &port,std::string status);
    //利用一致性哈希算法计算目标ip及其对应的key值
    void cal_hash_key();
    // 只有P/R两种状态，用于表示cache的身份
    std::string pr_status_;
    // IP port信息
    std::string status_, local_cache_IP_, port_for_client_, port_for_cache_;
    bool initial_flag;
    // TODO：测试用，待删除
    // int i = 0;
};

void addfd( int epollfd, int fd, bool enable_et );
int server_socket(std::string server_IP, std::string server_port);
int client_socket(std::string server_IP, std::string server_port);


//分割协议报文用的辅助函数：


#endif //CACHE_SOCK_CACHE_H