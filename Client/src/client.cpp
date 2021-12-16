#include "client.h"

typedef std::function<void (void *)> fp;

Client::Client(int cache_size_local, char mode, bool from_file) {
	mode_ = mode;
	from_file_ = from_file;

	cache_size_local_ = cache_size_local;
	local_lru_ = std::make_shared<
		cache::lru_cache<std::string, std::string>>(cache_size_local_);

	memset(&master_addr_, 0, sizeof(master_addr_));
	memset(&cache_sever_addr_, 0, sizeof(cache_sever_addr_));

	master_addr_.sin_family = PF_INET;
	master_addr_.sin_port = htons(MASTER_PORT);
	master_addr_.sin_addr.s_addr = inet_addr(MASTER_IP);

	cache_sever_addr_.sin_family = PF_INET;

	master_sock_ = 0;
	cache_sever_sock_ = 0;
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

	addfd(epollfd_, master_sock_, false);
	addfd(epollfd_, pipe_fd_[0], true);

	// master超时重传
	master_timer_ = std::make_shared<Timer> (WAITING_TIME, false, nullptr, 
																					(void*)&master_massage_);
	master_timer_->setCallback([](void * pData){
		std::string massage_tem = static_cast<ReSendMassage*> (pData)->massage;
		LOG(INFO) << "Master timeout retransmission, key: " + 
								massage_tem;
		if (send(static_cast<ReSendMassage*> (pData)->sock, 
				massage_tem.data(), BUF_SIZE, 0) == -1) {
			LOG(ERROR) << "Re-send to master error";
		}
	});
}

void Client::start() {
	int pid = fork();
	if (pid < 0) {
		LOG(ERROR) << "Fork error";
		close(master_sock_);
		exit(-1);
	} else if (pid == 0) {
		// 进入子进程，子进程负责随机产生key发送给父进程
		close(pipe_fd_[0]);

		std::string key;
		while (true) {
			if (from_file_) {
				std::ifstream key_list_file("./key_list.txt");
				if (!key_list_file.is_open()) { 
					LOG(ERROR) << "Open key_list file failed";
				}
				while(!key_list_file.eof()) { 
					usleep(REQUEST_INTERVAL * 1000);
					key_list_file >> key;
					std::cout << "Read from key_list file, key = " << key << std::endl;
					send_key_to_father(key, mode_);
				} 
				key_list_file.close();
			} else {
				usleep(REQUEST_INTERVAL * 1000);
				key = strRand(KEY_LENGTH);
				send_key_to_father(key, mode_);
			}
		}
	} else {
		// 进入父进程，父进程负责接收并处理来自子进程、master、cache server的消息
		close(pipe_fd_[1]);

		while(true) {
			int epoll_events_count = epoll_wait(epollfd_, events_, MAX_EVENT_NUMBER, -1);
			for (int i = 0; i < epoll_events_count; ++i) {
				if (events_[i].data.fd == pipe_fd_[0]) {
					// 子进程写入，message example: "CHILD#KEY"
					read(events_[i].data.fd, message_, BUF_SIZE);
					dealwith_child(message_);
					check_cache_server_request_map();  // TODO: 改为定时事件
				} else if (events_[i].data.fd == master_sock_) {
					// master消息，message example: "MASTER#KEY#ip:port"
					read(events_[i].data.fd, message_, BUF_SIZE);
					
					dealwith_master(message_);
				} else {
					// cache server消息，message example: SUCCESS/FAILED#key#ip:port 
					read(events_[i].data.fd, message_, BUF_SIZE);
					dealwith_cache_server(message_);
					close(events_[i].data.fd);  // DEBUG
					epoll_ctl(epollfd_, EPOLL_CTL_DEL, events_[i].data.fd, nullptr);
				}
			}
		}
	}	
}

void Client::connect_master() {
	if ((master_sock_ = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Socket Error is %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	// 连接master
	if (connect(master_sock_,
							(struct sockaddr *)(&master_addr_),
							sizeof(struct sockaddr)) == -1) {
		LOG(ERROR) << std::string("Connect master failed, ip: ") + 
									inet_ntoa(master_addr_.sin_addr);
		exit(EXIT_FAILURE);
	}
}

void Client::check_cache_server_request_map() {
	for(auto it = request_map_.begin(); it != request_map_.end(); ++it) {
		if (it->second.size() > LOST_MAX_NUM) {
			// 当有LOST_MAX_NUM个请求没有被响应时判定为宕机
			LOG(INFO) << "The cache server may be down: " + it->first;

			// 将未被响应的key重新向master请求分布
			for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
				// TODO: 协商数据包格式
				send_request_to_master(*it2);
				LOG(ERROR) << "Re-send to master, key: " + *it2;
				usleep(1000);
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

void Client::send_request_to_master(const std::string &key) {
	std::cout << "Request to master, key: " << key << std::endl;
	std::string send_tmp = key;
	send(master_sock_, send_tmp.data(), BUF_SIZE, 0);

	master_massage_.sock = master_sock_;
	master_massage_.massage = key;
	master_massage_.timer = master_timer_;
	master_timer_->reset();
	master_timer_->start();
}

void Client::send_request_to_cache_server(const std::string &addr,
																					const std::string &key, 
																					const std::string value) {

	add_item_to_request_map(addr, key);

	parse_str_addr(addr, cache_sever_addr_);

	if ((cache_sever_sock_ = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LOG(ERROR) << "Socket error";
		exit(EXIT_FAILURE);
	}
	std::string send_tmp = value != ""? key + "#" + value: key;
	if (connect(cache_sever_sock_,
						(struct sockaddr *)(&cache_sever_addr_),
						sizeof(struct sockaddr)) == -1) {
		LOG(ERROR) << "Connect cache server failed, addr: " + addr;
		close(cache_sever_sock_);
	} else {
		usleep(10 * 1000);  // DEBUG
		send(cache_sever_sock_, send_tmp.data(), send_tmp.size(), 0);
		addfd(epollfd_, cache_sever_sock_, true);
	}
	
	if (cache_server_timers_.find(addr) != cache_server_timers_.end()) {
		cache_server_timers_[addr].sock = cache_sever_sock_;
		cache_server_timers_[addr].massage = send_tmp;
		cache_server_timers_[addr].timer->reset();
		cache_server_timers_[addr].timer->start();
	} else {
		cache_server_massage_.addr = addr;
		cache_server_massage_.sock = cache_sever_sock_;
		cache_server_massage_.massage = send_tmp;
		cache_server_massage_.timer = 
			std::make_shared<Timer> (WAITING_TIME, false, nullptr, nullptr);
		cache_server_timers_.insert(std::make_pair(addr, cache_server_massage_));
		cache_server_timers_[addr].timer->setCallback(
			std::bind(&Client::cache_server_resend, this, std::placeholders::_1));
		cache_server_timers_[addr].timer->setData(&cache_server_timers_[addr]);
		cache_server_timers_[addr].timer->start();
	}
	std::cout << "Num of request to " + addr + " is " + 
							 std::to_string(++cache_server_timers_[addr].num) << std::endl;
}

void Client::dealwith_child(const char* message) {
	std::vector<std::string> message_array;
	std::string cache_server_addr;
	split(message, message_array, '#');

	std::string& key = message_array[1];
	if (message_array[0] == "CHILD") {
		//收到子进程发来的CHILD#KEY，检查本地Cache
		try {
			cache_server_addr = local_lru_->get(key);
			// 本地cache中有key-addr,向cache server发送读写请求
			
			if (mode_ == 'r') {
				send_request_to_cache_server(cache_server_addr, key);
			} else if (mode_ == 'w') {
				send_request_to_cache_server(cache_server_addr, key, strRand(VALUE_LENGTH));
			}
		}
		catch(const std::exception& e) {
			// 本地cache中没有key-addr,向master请求分布
			send_request_to_master(key);
		}
	}
}

void Client::dealwith_master(const char* message) {
	master_timer_->stop();

	// message example: MASTER#key#ip:port
	std::vector<std::string> message_array;
	split(message, message_array, '#');
	if (message_array.size() != 3) {
		LOG(ERROR) << "Recved message from Master error";
		return;
	}
	std::string& state = message_array[0];
	std::string& key = message_array[1];
	
	if (state == "MASTER") {
		std::string& addr = message_array[2];
		local_lru_->put(key, addr);  // 更新本地cache
		
		if (mode_ == 'r') {
			send_request_to_cache_server(addr, key);
		} else if (mode_ == 'w') {
			send_request_to_cache_server(addr, key, strRand(VALUE_LENGTH));
		}
	} else {
		LOG(INFO) << "Request to master failed, key: " + key;
	}
}

void Client::dealwith_cache_server(const char* message) {
	if(!message) {
		return;
	}
	// message example: SUCCESS/FAILED#key#ip:port
	std::vector<std::string> message_array;
	split(message, message_array, '#');
	std::string& state = message_array[0];
	std::string& key = message_array[1];
	std::string& addr = message_array[2];
	if (state == "SUCCESS") {
		erase_item_from_request_map(addr, key);
		cache_server_timers_[addr].timer->stop();
		if (message_array.size() == 3) {
			std::cout << "Request to cache server " + state + " key: " + key
								<< " cache addr: " + addr << std::endl;
		} else if (message_array.size() == 4) {
			std::cout << "Request to cache server " + state + " key: " + key
								<< " value: " + message_array[3]
								<< " cache addr: " + addr << std::endl;
		}
	} else if (state == "FAILED") {
		erase_item_from_request_map(addr, key);
		local_lru_->erase(key);  // 从本地缓存中删除
		cache_server_timers_[addr].timer->stop();
		std::cout << "Request to cache server " + state + " key: " + key
							<< " cache addr: " + addr << std::endl;
	}
}

void Client::add_item_to_request_map(const std::string &cache_server_addr, 
																		 const std::string &key) {
	if (request_map_.find(cache_server_addr) != request_map_.end()) {
		auto it = std::find(request_map_[cache_server_addr].begin(), 
												request_map_[cache_server_addr].end(), 
												key);
		if (it == request_map_[cache_server_addr].end()) {
			request_map_[cache_server_addr].emplace_back(key);
		}
	} else {
		std::list<std::string> tmp {key};
		request_map_.insert(std::make_pair(cache_server_addr, tmp));
	}
}

void Client::erase_item_from_request_map(const std::string &cache_server_addr, 
																				 const std::string &key) {
	if (request_map_.find(cache_server_addr) != request_map_.end()) {
		auto it = std::find(request_map_[cache_server_addr].begin(), 
												request_map_[cache_server_addr].end(), 
												key);
		if (it != request_map_[cache_server_addr].end()) {
			request_map_[cache_server_addr].erase(it);
		}
	}
}

void Client::cache_server_resend(void * pData) {
	std::string massage_tem = static_cast<ReSendMassage*> (pData)->massage;

	LOG(INFO) << "Cache server timeout retransmission, key = " + 
							 massage_tem;
							 
	parse_str_addr(static_cast<ReSendMassage*> (pData)->addr, cache_sever_addr_);
	
	int cache_sever_sock;
	if ((cache_sever_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LOG(ERROR) << "Socket error";
		exit(EXIT_FAILURE);
	}
	if (connect(cache_sever_sock,
						(struct sockaddr *)(&cache_sever_addr_),
						sizeof(struct sockaddr)) == -1) {
		LOG(ERROR) << "Re-connect cache server failed, addr: " + 
									(static_cast<ReSendMassage*> (pData))->addr;
		close(cache_sever_sock);
	} else {
		if (send(cache_sever_sock, massage_tem.data(), BUF_SIZE, 0) == -1) {
			LOG(ERROR) << "Re-send to cache server failed";
		} else {
			addfd(epollfd_, cache_sever_sock, true);
		}
	}
}
