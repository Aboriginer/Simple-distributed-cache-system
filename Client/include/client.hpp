#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "easylogging++.h"
#include "lru.hpp"
#include "tools.hpp"
#include "timer.hpp"

const int BUF_SIZE = 0xFFFF;  // 通信缓冲区大小

const char MASTER_IP[10] = "127.0.0.1";
const int MASTER_PORT = 8889;
const int CACHESEVER_PORT = 8887;

const int KEY_LENGTH = 3;
const int VALUE_LENGTH = 10;

const int MAX_EVENT_NUMBER = 100;

const int EPOLL_SIZE = 100;

const int REQUEST_INTERVAL = 1000;  // 产生读写请求间隔 ms

const int WAITING_TIME = 500;  // 超时重传等待时间 ms

// 超时重传回调函数接收的参数
struct ReSendMassage {
	std::string addr;
	int sock;
	std::string massage;
	Timer* timer;
};

class Client {

public:
	Client(int cache_size_local, char mode);
	void init();

	void start();

	void connect_master();

	// 检查cache_server的请求是否收到回复，主动发现cache_server宕机
	void check_cache_server_request_map();

	// 处理子进程发来的消息
	void dealwith_child(const char* message);

	// 处理master的返回信息
	void dealwith_master(const char* message);

	// 处理cache server的返回信息
	void dealwith_cache_server(const char* message);

	void send_key_to_father(const std::string key, const char mode);

	void send_request_to_cache_server(const std::string addr,
																		const std::string key, 
																		const std::string value = "");

	void send_request_to_master(const std::string my_addr,
															const std::string mod,
															const std::string key);

	void add_item_to_request_map(const std::string cache_server_addr, 
															 const std::string key);

	void erase_item_from_request_map(const std::string cache_server_addr, 
																	 const std::string key);

	void master_resend(void * pData);

	void cache_server_resend(void * pData);

private:
	// client的模式 -w write；-r read
	char mode_;

	epoll_event events[MAX_EVENT_NUMBER];

	int epollfd_;

	int master_sock;
	int cache_sever_sock;

	// fd[0]用于父进程随机生成key，检查本地cache后将key发送给子进程
	// fd[1]用于子进程接受父进程key
	int pipe_fd_[2];

	// 通信缓冲区
	char message[BUF_SIZE];

	struct sockaddr_in master_addr;
	struct sockaddr_in cache_sever_addr;

	// 本地Cache缓存条目大小
	int cache_size_local_;

	// 本地Cache缓存
	cache::lru_cache<std::string, std::string>* cache_lru;

	// 管理向cache_server发送的请求,ip-list key
	std::unordered_map<std::string, std::list<std::string>> cache_server_request_;

	// master重传定时器
	Timer *master_timer;

	ReSendMassage master_massage;
	ReSendMassage cache_server_massage;

	// cache server重传定时器表
	std::unordered_map<std::string, ReSendMassage> cache_server_timers;
};

#endif