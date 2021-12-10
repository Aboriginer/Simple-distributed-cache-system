#include "client.hpp"

typedef std::function<void (void *)> fp;

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

void Client::init() {
	epollfd_ = epoll_create(EPOLL_SIZE);
	// fd[0]用于父进程读，fd[1]用于子进程写
	if (pipe(pipe_fd_) < 0) {
		perror("pipe error");
		LOG(ERROR) << "Pipe error";
		exit(-1);
	}
	connect_master();

	addfd(epollfd_, master_sock, true);
	addfd(epollfd_, pipe_fd_[0], true);

	master_timer = new Timer(WAITING_TIME, false, NULL, (void*)&master_massage);
	master_timer->setCallback(std::bind(&Client::master_resend, 
																			this, 
																			std::placeholders::_1));
}

void Client::start() {
	int pid = fork();
	if (pid < 0) {
		LOG(ERROR) << "Fork error";
		close(master_sock);
		exit(-1);
	} else if (pid == 0) {
		// 进入子进程，子进程负责写入管道，关闭读端
		close(pipe_fd_[0]);

		while (true) {
			if (mode_ == 'r') {
				//read模式，读本地key_list文件
				std::ifstream key_list_file("./key_list.txt");
				std::string key;
				if (!key_list_file.is_open()) { 
					LOG(ERROR) << "Open key_list file failed";
				}
				while(!key_list_file.eof()) { 
					key_list_file >> key;
					send_key_to_father(key, 'r');
					std::cout << "Read from key_list file, key = " << key << std::endl;
					usleep(REQUEST_INTERVAL * 1000);
				} 
				key_list_file.close();
			} else if (mode_ == 'w') {
				std::string key = strRand(KEY_LENGTH);  // 随机生成Key
				send_key_to_father(key, 'w');
				std::cout << key << std::endl;
				usleep(REQUEST_INTERVAL * 1000);  // 微秒
			}
		}
	} else {
		// 进入父进程，父进程负责读管道数据，关闭写端
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
					// cache server返回是否读写成功
					int ret = read(events[i].data.fd, message, BUF_SIZE);
					dealwith_cache_server(message);
					close(events[i].data.fd);
					epoll_ctl(epollfd_, EPOLL_CTL_DEL, events[i].data.fd, NULL);
				}
			}
		}
	}	
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

void Client::check_cache_server_request_map() {
	for(auto it = cache_server_request_.begin(); it != cache_server_request_.end(); ++it) {
		if (it->second.size() > 5) {
			// 当有5个请求没有被响应时判定为宕机
			LOG(INFO) << "The cache server may be down: " + it->first;

			// 向master请求新的分布
			for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
				// TODO: 协商数据包格式
				send_request_to_master("my_addr", "WRITE", *it2);
				LOG(ERROR) << "Re-send to master, key: " + *it2;
			}
			it->second.clear();
		}
	}
}

void Client::send_key_to_father(const std::string key, const char mode) {
	if (write(pipe_fd_[1], ("CHILD#" + key).data(), BUF_SIZE) < 0) {
		LOG(ERROR) << "Pipe error";
		exit(-1);
	}
}

void Client::send_request_to_master(const std::string my_addr,
																		const std::string mod,
																		const std::string key) {
	// TODO: 协商数据包格式
	send(master_sock, key.data(), BUF_SIZE, 0);
	
	master_massage.sock = master_sock;
	master_massage.massage = key;
	master_massage.timer = master_timer;
	master_timer->reset();
	master_timer->start();
}

void Client::send_request_to_cache_server(const std::string addr,
																					const std::string key, 
																					const std::string value) {
	add_item_to_request_map(addr, key);

	parse_str_addr(addr, cache_sever_addr);

	if ((cache_sever_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LOG(ERROR) << "Socket error";
		exit(EXIT_FAILURE);
	}
	std::string send_tmp = value != ""? key + "#" + value: key;
	if (connect(cache_sever_sock,
						(struct sockaddr *)(&cache_sever_addr),
						sizeof(struct sockaddr)) == -1) {
		LOG(ERROR) << "Connect cache server failed, addr: " + addr;
	} else {
		send(cache_sever_sock, send_tmp.data(), send_tmp.size(), 0);
		addfd(epollfd_, cache_sever_sock, true);
	}
	
	if (cache_server_timers.find(addr) != cache_server_timers.end()) {
		cache_server_timers[addr].sock = cache_sever_sock;
		cache_server_timers[addr].massage = send_tmp;
		cache_server_timers[addr].timer->reset();
		cache_server_timers[addr].timer->start();
	} else {
		cache_server_massage.addr = addr;
		cache_server_massage.sock = cache_sever_sock;
		cache_server_massage.massage = send_tmp;
		cache_server_massage.timer = new Timer(WAITING_TIME, false, NULL, NULL);
		cache_server_timers.insert(std::make_pair(addr, cache_server_massage));
		cache_server_timers[addr].timer->setCallback(
			std::bind(&Client::cache_server_resend, this, std::placeholders::_1));
		cache_server_timers[addr].timer->setData(&cache_server_timers[addr]);
		cache_server_timers[addr].timer->start();
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
	master_timer->stop();
	// message example: SUCCESS#key#ip:port / FAILED#key
	std::vector<std::string> message_array;
	split(message, message_array, '#');
	std::string& state = message_array[0];
	std::string& key = message_array[1];
	
	if (state == "SUCCESS") {
		std::string& addr = message_array[2];
		cache_lru->put(key, addr);  //更新本地cache
		if (mode_ == 'r') {
			send_request_to_cache_server(addr, key);
		} else if (mode_ == 'w') {
			send_request_to_cache_server(addr, key, strRand(20));
		}
	} else if (state == "FAILED") {
		LOG(INFO) << "Request to master failed, key: " + key;
	}
}

void Client::dealwith_cache_server(const char* message) {
	// message example: SUCCESS/FAILED#key#ip:port
	std::vector<std::string> message_array;
	split(message, message_array, '#');
	std::string& state = message_array[0];
	std::string& key = message_array[1];
	std::string& addr = message_array[2];
	if (state == "SUCCESS" || state == "FAILED") {
		erase_item_from_request_map(addr, key);
		cache_server_timers[addr].timer->stop();
		std::cout << "Request " + state + " key = " + key 
							<< " addr = " + addr << std::endl;
	}
}

void Client::add_item_to_request_map(const std::string cache_server_addr, 
																		 const std::string key) {
	if (cache_server_request_.find(cache_server_addr) != cache_server_request_.end()) {
		auto it = std::find(cache_server_request_[cache_server_addr].begin(), 
												cache_server_request_[cache_server_addr].end(), 
												key);
		if (it == cache_server_request_[cache_server_addr].end()) {
			cache_server_request_[cache_server_addr].emplace_front(key);
		}
	} else {
		std::list<std::string> tmp {key};
		cache_server_request_.insert(std::make_pair(cache_server_addr, tmp));
	}
}

void Client::erase_item_from_request_map(const std::string cache_server_addr, 
																				 const std::string key) {
	if (cache_server_request_.find(cache_server_addr) != cache_server_request_.end()) {
		auto it = std::find(cache_server_request_[cache_server_addr].begin(), 
												cache_server_request_[cache_server_addr].end(), 
												key);
		if (it != cache_server_request_[cache_server_addr].end()) {
			cache_server_request_[cache_server_addr].erase(it);
		}
	}
}

void Client::master_resend(void * pData) {
	std::string massage_tem = ((ReSendMassage*) pData)->massage;

	LOG(INFO) << "Master timeout retransmission" + 
							 massage_tem;

	if (send(((ReSendMassage*) pData)->sock, 
					 massage_tem.data(), BUF_SIZE, 0) == -1) {
		LOG(ERROR) << "Re-send to master error";
	};
}

void Client::cache_server_resend(void * pData) {
	std::string massage_tem = ((ReSendMassage*) pData)->massage;

	LOG(INFO) << "Cache server timeout retransmission, key = " + 
							 massage_tem;
							 
	parse_str_addr(((ReSendMassage*) pData)->addr, cache_sever_addr);
	
	int cache_sever_sock;
	if ((cache_sever_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LOG(ERROR) << "Socket error";
		exit(EXIT_FAILURE);
	}
	if (connect(cache_sever_sock,
						(struct sockaddr *)(&cache_sever_addr),
						sizeof(struct sockaddr)) == -1) {
		LOG(ERROR) << "Re-connect cache server failed, addr: " + 
									((ReSendMassage*) pData)->addr;
		close(cache_sever_sock);
	} else {
		if (send(cache_sever_sock, massage_tem.data(), BUF_SIZE, 0) == -1) {
			LOG(ERROR) << "Re-send to cache server failed";
		} else {
			addfd(epollfd_, cache_sever_sock, true);
		}
	}
}
