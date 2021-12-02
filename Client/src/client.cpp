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
	connect_master();
	if (mode_ == 'w') {
        Write();
    } else if (mode_ == 'r'){
        Read();
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
	while (true) {
		//随机生成K V
		std::string key = strRand(KEY_LENGTH);
		std::string value = strRand(VALUE_LENGTH);

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
		//写入Cache Server
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
		strcpy(send_buff, (key + "#" + value).data()); //使用"#"分隔K V
		send(cache_sever_sock, send_buff, BUF_SIZE, 0);
		//等待Cache Server返回True or False
		recv(cache_sever_sock, recv_buff, BUF_SIZE, 0);
		if (strcmp("True", recv_buff) == 0) {
			std::cout << "Put successful" << std::endl;
		} else {
			std::cout << "Put failed" << std::endl;
		}
		close(cache_sever_sock);

		sleep(1); //TODO 使用更精确的定时方法
	}
}
