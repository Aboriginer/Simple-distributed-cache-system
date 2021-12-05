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
	cache_sever_addr.sin_port = htons(CACHESEVER_PORT);

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
		fprintf(stderr, "Connect failed\n");
		exit(EXIT_FAILURE);
	}
}

void Client::Start() {
	epollfd_ = epoll_create(EPOLL_SIZE);
	// 创建管道，其中fd[0]用于父进程读，fd[1]用于子进程写
	if (pipe(pipe_fd_) < 0)
	{
		perror("pipe error");
		exit(-1);
	}
	connect_master();

	//将master_sock和管道读端描述符添加到内核事件表中
	addfd(epollfd_, master_sock, true);
	addfd(epollfd_, pipe_fd_[0], true);

	if (mode_ == 'w') {
        Write();
    } else if (mode_ == 'r'){
        Read();
    }
}

void Client::check_cache_server_request() {
	for(auto it = cache_server_request_.begin(); it != cache_server_request_.end(); ++it) {
		if(it->second > 5) {
			std::cout << "The cache server may be down: " << it->first.data() << std::endl;
		}
	}
}

void Client::Read() {
	//TODO: 写入重要日志

	//TODO: 读取keylist文件
	std::string key = strRand(KEY_LENGTH);
	std::string cache_server_ip = "";
	while (true) {
		std::string key = strRand(KEY_LENGTH);
		// std::cout << key << std::endl;

		cache_server_ip = "";
		try {
			cache_server_ip = cache_lru->get(key);
			cache_sever_addr.sin_addr.s_addr = inet_addr(cache_server_ip.data());
		}
		catch (const std::exception &e) {
			std::cerr << e.what() << '\n';
			//向master请求分布
			strcpy(send_buff, key.data());
			send(master_sock, send_buff, BUF_SIZE, 0);
			recv(master_sock, recv_buff, BUF_SIZE, 0);

			std::cout << "Key: " << key << "   "
								<< "CacheSever IP: "
								<< recv_buff << std::endl;

			//TODO: 解析recv_buff
			cache_sever_addr.sin_addr.s_addr = inet_addr(recv_buff);
			//写入本地缓存
			cache_lru->put(key, recv_buff);
		}

		if ((cache_sever_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			fprintf(stderr, "Socket Error is %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		// 向CacheSever请求value
		if (connect(cache_sever_sock,
								(struct sockaddr *)(&cache_sever_addr),
								sizeof(struct sockaddr)) == -1) {
			fprintf(stderr, "Connect failed\n");
			exit(EXIT_FAILURE);
		}

		strcpy(send_buff, key.data());
		send(cache_sever_sock, send_buff, BUF_SIZE, 0);
		recv(cache_sever_sock, value_, BUF_SIZE, 0);
		close(cache_sever_sock);
		bzero(send_buff, BUF_SIZE);
		cache_lru->put(key, value_);
		std::cout << "Key: " << key << "   "
							<< "Value: " << value_ << std::endl;

		sleep(1);
	}
}

void Client::Write() {
	std::string cache_server_ip = "";
	// 向master请求分布、
  // 向cache_sever、发送读写请求、接收cache_sever返回
	
	// 创建子进程
	pid = fork();

	// 如果创建子进程失败则退出
	if (pid < 0)
	{
		perror("fork error");
		close(master_sock);
		exit(-1);
	}
	else if (pid == 0)
	{
		// 进入子进程
		// 子进程负责写入管道，因此先关闭读端
		close(pipe_fd_[0]);

		check_cache_server_request();

		std::cout << "write mode" << std::endl;

		while (true) {
		//随机生成Key
		std::string key = strRand(KEY_LENGTH);

		cache_server_ip = "";
		try {
			cache_server_ip = cache_lru->get(key);
			// 本地cache中有key-addr,向父进程发送key#addr
			if (write(pipe_fd_[1], (key + "#" + cache_server_ip).data(), 
								strlen(cache_server_ip.data()) - 1) < 0) {
				perror("fork error");
				exit(-1);
			}
		}
		catch (const std::exception &e) {
			// 本地cache中没有key-addr,向父进程发送key#REQUEST
			std::cerr << e.what() << '\n';
			if (write(pipe_fd_[1], (key + "#REQUEST").data(), BUF_SIZE) < 0) {
				perror("fork error");
				exit(-1);
			}
		}
		sleep(5); //TODO 使用更精确的定时方法
		}  // while (true)
	} else {
		// 进入父进程
		// 父进程负责读管道数据，因此先关闭写端
		close(pipe_fd_[1]);

		std::vector<std::string> key_addr_value;

		// cache_server的返回，写请求：addr#状态#key，读请求：addr#状态#key#value
		std::vector<std::string> cache_server_recv;

		while(true) {
			int epoll_events_count = epoll_wait(epollfd_, events, MAX_EVENT_NUMBER, -1);

			// 处理就绪事件
			for (int i = 0; i < epoll_events_count; ++i) {
				if (events[i].data.fd == pipe_fd_[0] || events[i].data.fd == master_sock) {
					// 子进程写入事件/master返回分布事件
					int ret = read(events[i].data.fd, message, BUF_SIZE);
					split(message, key_addr_value, '#');  // 解析message

					if (key_addr_value[1] == "REQUEST") {
						//收到子进程发来的key#REQUEST，向master请求分布
						strcpy(send_buff, key_addr_value[0].data());
						send(master_sock, send_buff, BUF_SIZE, 0);
					} else {
						//收到子进程或master发来的key#addr，向cache_server发送读写请求
						auto it = cache_server_request_.find(key_addr_value[1]);
						if(it != cache_server_request_.end()) {
							++cache_server_request_[key_addr_value[1]];
						} else {
							cache_server_request_.insert(std::make_pair(key_addr_value[1], 0));
						}

						//更新本地cache
						cache_lru->put(key_addr_value[0], key_addr_value[1]);

						//向cache_server发送写请求
						cache_sever_addr.sin_addr.s_addr = inet_addr(key_addr_value[1].data());
						if ((cache_sever_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
						fprintf(stderr, "Socket Error is %s\n", strerror(errno));
						exit(EXIT_FAILURE);
						}
						if (connect(cache_sever_sock,
											(struct sockaddr *)(&cache_sever_addr),
											sizeof(struct sockaddr)) == -1) {
						fprintf(stderr, "Connect failed\n");
						exit(EXIT_FAILURE);
						}
						//TODO: 添加连接cache server超时处理
						strcpy(send_buff, (key_addr_value[0] + "#" + strRand(VALUE_LENGTH)).data()); //使用"#"分隔K V
						send(cache_sever_sock, send_buff, BUF_SIZE, 0);
						addfd(epollfd_, cache_sever_sock, true);
					}
					
				} else {
					// cache_server返回请求状态，SUCCESS/FAILED
					const int current_cache_server_fd = events[i].data.fd;
					int ret = read(current_cache_server_fd, message, BUF_SIZE);
					split(message, cache_server_recv, '#');
					
					if(cache_server_recv[1] == "SUCCESS") {
						std::cout << "Write key: " + cache_server_recv[2] + ",value: " +
												 cache_server_recv[3] + " to " + cache_server_recv[0] + 
												 " success."<< std::endl;
					} else if (cache_server_recv[1] == "FAILED") {
						std::cout << "Write to " + cache_server_recv[0] + " failed."<< std::endl;
					}

					if(--cache_server_request_[cache_server_recv[0]] == 0) {
							cache_server_request_.erase(cache_server_recv[0]);
							close(current_cache_server_fd);
							//???关闭连接后需要移除事件吗???
							struct epoll_event ev;
							epoll_ctl(epollfd_, EPOLL_CTL_DEL, current_cache_server_fd, &ev);
					}
				}
			}
		}
	}	
}
