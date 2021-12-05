#include <algorithm>

#include "client.hpp"

Client::Client(int cache_size_local,char mode) {
	mode_ = mode;
	
	cache_size_local_ = cache_size_local;
	cache_lru = new cache::lru_cache<std::string, std::string>(cache_size_local_);

	bzero(&master_addr, sizeof(master_addr));
	bzero(&cache_sever_addr, sizeof(cache_sever_addr));

	master_addr.sin_family = PF_INET;
	master_addr.sin_port = htons(MASTER_PORT);
	master_addr.sin_addr.s_addr = inet_addr(MASTER_IP);

	cache_sever_addr.sin_family = PF_INET;

	master_sock = 0;
	cache_sever_sock = 0;
}

void Client::connect_master() {
	if ((master_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Socket Error is %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	// 连接master
	if (connect(master_sock,
							(struct sockaddr *)(&master_addr),
							sizeof(struct sockaddr)) == -1) {
		LOG(ERROR) << "Connect master failed, ip: " + std::to_string(master_addr.sin_port);
		exit(EXIT_FAILURE);
	}
}

void Client::init() {
	epollfd_ = epoll_create(EPOLL_SIZE);
	// 创建管道，其中fd[0]用于父进程读，fd[1]用于子进程写
	if (pipe(pipe_fd_) < 0) {
		perror("pipe error");
		LOG(ERROR) << "Pipe error";
		exit(-1);
	}
	connect_master();

	//将master_sock和管道读端描述符添加到内核事件表中
	addfd(epollfd_, master_sock, true);
	addfd(epollfd_, pipe_fd_[0], true);
}

void Client::check_cache_server_request_map() {
	for(auto it = cache_server_request_.begin(); it != cache_server_request_.end(); ++it) {
		std::cout << it->first << std::endl;
		if(it->second.size() > 5) {
			//当有5个请求没有被响应时判定为宕机
			LOG(INFO) << "The cache server may be down: " + it->first;
		}
	}
}

void Client::start() {
	pid = fork();
	if (pid < 0) {
		LOG(ERROR) << "Fork error";
		close(master_sock);
		exit(-1);
	} else if (pid == 0) {
		// 进入子进程，子进程负责写入管道，先关闭读端
		close(pipe_fd_[0]);

		LOG(INFO) << "write mode";

		while (true) {
			//随机生成Key
			std::string key = strRand(KEY_LENGTH);
			if (write(pipe_fd_[1], ("CHILD#" + key).data(), BUF_SIZE) < 0) {
					LOG(ERROR) << "Pipe error";
					exit(-1);
			}
			sleep(1); //TODO 使用更精确的定时方法
		}  // while (true)
	} else {
		// 进入父进程，父进程负责读管道数据，因此先关闭写端
		close(pipe_fd_[1]);

		while(true) {
			int epoll_events_count = epoll_wait(epollfd_, events, MAX_EVENT_NUMBER, -1);
			// 处理就绪事件
			for (int i = 0; i < epoll_events_count; ++i) {
				if (events[i].data.fd == pipe_fd_[0]) {
					// 子进程写入，message example: "CHILD#KEY"
					int ret = read(events[i].data.fd, message, BUF_SIZE);
					dealwith_child(message);
					// TODO: 改为定时事件
					check_cache_server_request_map();
				} else if (events[i].data.fd == master_sock) {
					// master返回分布，message example: "KEY#ip:port"
					int ret = read(events[i].data.fd, message, BUF_SIZE);
					dealwith_master(message);
				} else {
					// cache server返回，message example: "ip:port#state#key#value"
					int ret = read(events[i].data.fd, message, BUF_SIZE);
					dealwith_cache_server(message);
					close(events[i].data.fd);
					epoll_ctl(epollfd_, EPOLL_CTL_DEL, events[i].data.fd, NULL);
				}
			}
		}
	}	
}

void Client::dealwith_child(const char* message) {
	std::vector<std::string> message_array;
	std::string cache_server_addr;
	split(message, message_array, '#');

	std::string& key = message_array[1];
	if (message_array[0] == "CHILD") {
		//收到子进程发来的CHILD#KEY，检查本地Cache
		try {
			cache_server_addr = cache_lru->get(key);
			// 本地cache中有key-addr,向cache server发送读写请求
			if (mode_ == 'r') {
				send_request_to_cache_server(cache_server_addr, key);
			} else if (mode_ == 'w') {
				send_request_to_cache_server(cache_server_addr, key, strRand(20));
			}
		}
		catch(const std::exception& e) {
			// 本地cache中没有key-addr,向master请求分布
			send_request_to_master("my_addr", "WRITE", key);
		}
	}
}

void Client::dealwith_master(const char* message) {
	std::vector<std::string> message_array;
	split(message, message_array, '#');
	// message example: "KEY#ip:port"
	std::string& key = message_array[0];
	std::string& addr = message_array[1];

	cache_lru->put(key, addr);  //更新本地cache

	if (mode_ == 'r') {
		send_request_to_cache_server(addr, key);
	} else if (mode_ == 'w') {
		send_request_to_cache_server(addr, key, strRand(20));
	}
}

void Client::dealwith_cache_server(const char* message) {
	// message example: ip:port#state#key(#value)
	std::vector<std::string> message_array;
	split(message, message_array, '#');
	std::string& addr = message_array[0];
	std::string& state = message_array[1];
	std::string& key = message_array[2];
	
	if(state == "SUCCESS") {
		erase_item_from_request_map(addr, key);
		std::cout << "Request succeed. key = " + key 
							<< " addr = " + addr << std::endl;
	} else if (state == "FAILED") {
		std::cout << "Request failed. key = " + key
							<< " addr = " + addr << std::endl;
	}
}

void Client::send_request_to_master(std::string my_addr,
																		std::string mod,
																		std::string key) {
	// TODO: 协商数据包格式
	send(master_sock, key.data(), BUF_SIZE, 0);
}

void Client::send_request_to_cache_server(std::string addr,
																					std::string key, 
																					std::string value) {
	add_item_to_request_map(addr, key);

	std::vector<std::string> ip_port;
	split(addr, ip_port, ':');
	cache_sever_addr.sin_addr.s_addr = inet_addr(ip_port[0].data());
	cache_sever_addr.sin_port = htons(atol(ip_port[1].data()));

	if ((cache_sever_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LOG(ERROR) << "Socket error";
		exit(EXIT_FAILURE);
	}

	if (connect(cache_sever_sock,
						(struct sockaddr *)(&cache_sever_addr),
						sizeof(struct sockaddr)) == -1) {
		//TODO: 添加连接cache server超时处理
		LOG(ERROR) << "Connect cache server failed, addr: " + 
									ip_port[0] + ":" + ip_port[1];
	} else {
		std::string send_tmp = key + "#" + value;
		send(cache_sever_sock, send_tmp.data(), send_tmp.size(), 0);
		addfd(epollfd_, cache_sever_sock, true);
	}
}

void Client::add_item_to_request_map(std::string cache_server_addr, 
																		 std::string key) {
	auto it = cache_server_request_.find(cache_server_addr);
	if(it != cache_server_request_.end()) {
		cache_server_request_[cache_server_addr].emplace_front(key);
	} else {
		std::list<std::string> tmp {key};
		cache_server_request_.insert(std::make_pair(cache_server_addr, tmp));
	}
}

void Client::erase_item_from_request_map(std::string cache_server_addr, std::string key) {
	if(cache_server_request_.find(cache_server_addr) != cache_server_request_.end()) {
		auto it = std::find(cache_server_request_[cache_server_addr].begin(), 
												cache_server_request_[cache_server_addr].end(), 
												key);
		if(it != cache_server_request_[cache_server_addr].end()) {
			cache_server_request_[cache_server_addr].erase(it);
		}
	}
}
