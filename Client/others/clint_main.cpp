



		// cache_server_ip_port = "";
		// try {
		// 	cache_server_ip_port = cache_lru->get(key);
		// 	// 本地cache中有key-addr,向父进程发送key#addr
		// 	if (write(pipe_fd_[1], (key + "#" + cache_server_ip_port).data(), 
		// 						strlen(cache_server_ip_port.data()) - 1) < 0) {
		// 		LOG(ERROR) << "Pipe error";
		// 		exit(-1);
		// 	}
		// }
		// catch (const std::exception &e) {
		// 	// 本地cache中没有key-addr,向父进程发送key#REQUEST
		// 	// std::cerr << e.what() << '\n';
		// 	if (write(pipe_fd_[1], (key + "#REQUEST").data(), BUF_SIZE) < 0) {
		// 		LOG(ERROR) << "Pipe error";
		// 		exit(-1);
		// 	}
		// }

					// if (key_addr_value[0] == "CHILD") {
					// 	//收到子进程发来的CHILD#KEY，检查本地Cache
					// 	try
					// 	{
					// 		cache_server_ip_port = cache_lru->get(key_addr_value[1]);
					// 		// 本地cache中有key-addr,向父进程发送key#addr
					// 		if (write(pipe_fd_[1], (key + "#" + cache_server_ip_port).data(), 
					// 							strlen(cache_server_ip_port.data()) - 1) < 0) {
					// 			LOG(ERROR) << "Pipe error";
					// 			exit(-1);
					// 		}
					// 	}
					// 	catch(const std::exception& e)
					// 	{
					// 		std::cerr << e.what() << '\n';
					// 	}
						
					// 	strcpy(send_buff, key_addr_value[0].data());
					// 	send(master_sock, send_buff, BUF_SIZE, 0);




				// 	//收到子进程或master发来的key#addr，向cache_server发送读写请求
				// 	auto it = cache_server_request_.find(key_addr_value[1]);
				// 	if(it != cache_server_request_.end()) {
				// 		cache_server_request_[key_addr_value[1]].emplace_front(key_addr_value[0]);
				// 	} else {
				// 		std::list<std::string> tmp {key_addr_value[0]};
				// 		cache_server_request_.insert(std::make_pair(key_addr_value[1], tmp));
				// 	}

				// 	//更新本地cache
				// 	cache_lru->put(key_addr_value[0], key_addr_value[1]);

				// 	//连接cache_server并发送请求
				// 	split(key_addr_value[1], ip_port, ':');
				// 	cache_sever_addr.sin_addr.s_addr = inet_addr(ip_port[0].data());
				// 	cache_sever_addr.sin_port = htons(atol(ip_port[1].data()));
					
				// 	if ((cache_sever_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
				// 	fprintf(stderr, "Socket Error is %s\n", strerror(errno));
				// 	exit(EXIT_FAILURE);
				// 	}
				// 	if (connect(cache_sever_sock,
				// 						(struct sockaddr *)(&cache_sever_addr),
				// 						sizeof(struct sockaddr)) == -1) {
				// 		//TODO: 添加连接cache server超时处理
				// 		LOG(ERROR) << "Connect cache server failed, addr: " + 
				// 									ip_port[0] + ":" + ip_port[1];
				// 		// exit(EXIT_FAILURE);
				// 	} else {
				// 		std::string send_tmp;
				// 		if (mode_ == 'w') {
				// 			send_tmp = key_addr_value[0] + "#" + strRand(VALUE_LENGTH);
				// 		} else if (mode_ == 'r') {
				// 			send_tmp = key_addr_value[0];
				// 		}
				// 		strcpy(send_buff, send_tmp.data()); //使用"#"分隔K V
				// 		send(cache_sever_sock, send_buff, BUF_SIZE, 0);
				// 		addfd(epollfd_, cache_sever_sock, true);
				// 	}
				// 	}
				// } else {
				// 	// cache_server返回请求状态，SUCCESS/FAILED
				// 	const int current_cache_server_fd = events[i].data.fd;
				// 	int ret = read(current_cache_server_fd, message, BUF_SIZE);
				// 	split(message, cache_server_recv, '#');
				// 	std::string& addr = cache_server_recv[0];
				// 	std::string& state = cache_server_recv[1];
				// 	std::string& key = cache_server_recv[2];
					
				// 	if(state == "SUCCESS") {
				// 		std::cout << "Request succeed. key = " + state 
				// 							<< " addr = " + addr << std::endl;
				// 	} else if (state == "FAILED") {
				// 		std::cout << "Request failed. key = " + state
				// 							<< " addr = " + addr << std::endl;
				// 	}

				// 	if(cache_server_request_.find(addr) != cache_server_request_.end()) {
				// 		auto it = std::find(cache_server_request_[addr].begin(), 
				// 												cache_server_request_[addr].end(), 
				// 												key);
				// 		if(it != cache_server_request_[addr].end()) {
				// 			cache_server_request_[addr].erase(it);
				// 		}
				// 	}
				// }

