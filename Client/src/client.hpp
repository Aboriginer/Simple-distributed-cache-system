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
#include <vector>

#include "easylogging++.h"
#include "lru.hpp"
#include "rand_string.hpp"

// 缓冲区大小65535
const int BUF_SIZE = 0xFFFF;

// Master IP地址
const char MASTER_IP[10] = "127.0.0.1";

// 服务器端口号
const int MASTER_PORT = 8889;
const int CACHESEVER_PORT = 8887;

const int KEY_LENGTH = 3;
const int VALUE_LENGTH = 10;

const int MAX_EVENT_NUMBER = 10000; //最大事件数

const int EPOLL_SIZE = 10;  // epoll支持的最大句柄数

class Client {

public:
	Client(int cache_size_local, char mode);
	void init();

	void start();

	void connect_master();

	//检查cache_server的请求是否收到回复，主动发现cache_server宕机
	void check_cache_server_request_map();

	void dealwith_child(const char* message);

	void dealwith_master(const char* message);

	void dealwith_cache_server(const char* message);

	void send_request_to_cache_server(std::string addr,
																		std::string key, 
																		std::string value = "");

	void send_request_to_master(std::string my_addr,
															std::string mod,
															std::string key);

	void add_item_to_request_map(std::string cache_server_addr, std::string key);

	void erase_item_from_request_map(std::string cache_server_addr, std::string key);

private:
	char mode_;  //client模式 -w write；-r read

	epoll_event events[MAX_EVENT_NUMBER];
	int epollfd_;

	int master_sock;
	int cache_sever_sock;

	// fd[0]用于父进程随机生成key，检查本地cache后将key发送给子进程
	// fd[1]用于子进程接受父进程key
	int pipe_fd_[2];

	int pid;  //当前进程ID

	//通信缓冲区
	char message[BUF_SIZE];

	struct sockaddr_in master_addr;
	struct sockaddr_in cache_sever_addr;

	//本地Cache缓存条目大小
	int cache_size_local_;

	//本地Cache缓存
	cache::lru_cache<std::string, std::string>* cache_lru;

	//管理向cache_server发送的请求,ip-list key
	std::unordered_map<std::string, std::list<std::string>> cache_server_request_;
};

#endif