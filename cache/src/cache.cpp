//
// Created by Aboriginer on 12/1/2021.
//

#include "cache.h"

Cache::Cache(int cache_size_local, std::string status, std::string local_cache_IP, std::string port_for_client,
             std::string port_for_cache) {
    cache_size_local_ = cache_size_local;
    status_ = status;
    local_cache_IP_ = local_cache_IP;
    port_for_client_ = port_for_client;
    port_for_cache_ = port_for_cache;
}

void Cache::Start() {
    auto Heartbeat_bind = std::bind(&Cache::Heartbeat,  this);
    auto Client_chat_bind = std::bind(&Cache::Client_chat, this);
//    auto cache_pass_bind = std::bind(&Cache::cache_pass, this);

    std::thread for_master_Heartbeat(Heartbeat_bind);
//    std::thread for_master_chat(Master_chat);
    std::thread for_client(Client_chat_bind);
//    std::thread for_cache_pass(cache_pass_bind);
//    std::thread for_replica(ToReplica);


    for_master_Heartbeat.join();
//    for_master_chat.join();
    for_client.join();
//    for_cache_pass.join();
//    for_replica.join();
}

void Cache::replica_chat() {
        while (true) {
            if (status_ == "P") {   // 作为服务器端
                if (replica_IP_ != "None") {    // 确定备份cache已上线

                }
            }   else {  // 作为客户端

            }
        }

//        int cache_rep_sock;
//        char rep_buff_master[BUF_SIZE];
//
//        struct sockaddr_in master_addr;
//        master_addr.sin_family = PF_INET;
//        master_addr.sin_port = htons(replicaPort);
//        master_addr.sin_addr.s_addr = inet_addr(replicaIP);
//
//        if ((cache_rep_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
//            fprintf(stderr, "Socket Error is %s\n", strerror(errno));
//            exit(EXIT_FAILURE);
//        }
//
//        if (accept(cache_rep_sock,(struct sockaddr *)(&master_addr), sizeof(struct sockaddr)) == -1) {
//            fprintf(stderr, "Connect failed\n");
//            exit(EXIT_FAILURE);
//        }
//        while(true){
//            if(bufferReplica.size() > 0){
//                memcpy(rep_buff_master, bufferReplica.c_str(), bufferReplica.size());
//                send(cache_rep_sock, rep_buff_master, BUF_SIZE, 0);
//            }else{
//                continue;
//            }
//        }
}
//
//
//void Cache::CachePass() {
//    int cache_cache_sock_in;
//    int cache_cache_sock_out;
//    // int
//    struct sockaddr_in master_addr;
//    char send_buff_master[BUF_SIZE];
//    // Master返回的扩缩容信息
//    char recv_buff_master[BUF_SIZE];
//    master_addr.sin_family = PF_INET;
//    master_addr.sin_port = htons(CACHE_PORT);
//    master_addr.sin_addr.s_addr = inet_addr(MASTER_IP);
//
//    auto pass = [](std::string key)->std::string{
//        std::string val = MainCache.get(key);
//        std::string res = key + "#" + val;
//        MainCache.del(key);
//        return res;
//    };
//
//    auto duplicate[](std::string key)->std::string{
//        std::string val = MainCache.get(key);
//        std::string res = key + "#" + val;
//        return res;
//    };
//
//    auto getval[](std::string message)->int{
//        int spear = 0;
//        for (int i = 0; i < message.size(); i++) {
//            if (message[i] != '#') {
//                ++spear;
//            }
//            else {
//                break;
//            }
//        }
//        std::string key = message.substr(0, spear);
//        std::string val = message.substr(spear + 1, message.size());
//        MainCache.put(key, val);
//        return 0;
//    }
//
//    if ((cache_cache_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
//        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
//        exit(EXIT_FAILURE);
//    }
//    // 连接master
//    if (connect(cache_cache_sock,(struct sockaddr *)(&master_addr), sizeof(struct sockaddr)) == -1) {
//        fprintf(stderr, "Connect failed\n");
//        exit(EXIT_FAILURE);
//    }
//
//    if (accept(cache_cache_sock,(struct sockaddr *)(&master_addr), sizeof(struct sockaddr)) == -1) {
//        fprintf(stderr, "Connect failed\n");
//        exit(EXIT_FAILURE);
//    }
//
//
//    if (listen(serv_client_sock, 5) == -1) {
//        fprintf(stderr, "listen error\n");
//        exit(EXIT_FAILURE);
//    }
//
//
//    while (true) {
//        if(status == 0){         //什么事都没有发生
//            // sleep(3);
//            continue
//        }
//        if(status == 1){         //向下一个cache传递信息
//            std::cout << "Start send message" << std::endl;
//            std::cout << "Send message:" << send_buff_master << std::endl;
//            // string passe
//            string buffer = pass(key);
//
//            send(cache_cache_sock_in, send_buff_master, BUF_SIZE, 0);
//            std::cout << "Send heartbeat ok" << std::endl;
//            bzero(send_buff_master, BUF_SIZE);
//            // sleep(3);
//            // continue
//        }else if(status == 2){  //从上一个cache接受信息
//            //TODO : 这块好像没写明白。
//            std::cout <<"receving message"<<std::endl;
//            recv(cache_cache_sock_out, recv_buff_client, BUF_SIZE, 0);
//            getval(recv_buff_client);
//            bzero(recv_buff_client, BUF_SIZE);
//            continue;
//        }
//    }
//
//
//}


void Cache::Heartbeat() {
    int cache_master_sock;
    struct sockaddr_in master_addr;
    // 向master发送的心跳包
    char send_buff_master[BUF_SIZE];

    bzero(&master_addr, sizeof(master_addr));

    master_addr.sin_family = PF_INET;
    master_addr.sin_port = htons(MASTER_PORT);
    master_addr.sin_addr.s_addr = inet_addr(MASTER_IP);

    Timer *timer = new Timer();//500ms上传一次心跳包

    if ((cache_master_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // 连接master
    if (connect(cache_master_sock, (struct sockaddr *) (&master_addr), sizeof(struct sockaddr)) == -1) {
        fprintf(stderr, "Connect failed\n");
        exit(EXIT_FAILURE);
    }

    while (true) {
        // 发送心跳
        std::string heart_message = "x#" + local_cache_IP_ + "#" + port_for_client_ + "#" + status_;

        strcpy(send_buff_master, heart_message.data());

        std::cout << "Send message:" << send_buff_master << std::endl;

        send(cache_master_sock, send_buff_master, BUF_SIZE, 0);

        std::cout << "Heartbeat successfully!" << std::endl;
        bzero(send_buff_master, BUF_SIZE);

        timer->start();
        while(timer->isRunning());
        timer->stop();
    }
    close(cache_master_sock);
}


//void Cache::Master_chat() {
//    int cache_master_sock;
//    struct sockaddr_in master_addr;
//    // Master返回的扩缩容信息
//    char recv_buff_master[BUF_SIZE];
//
//
//    bzero(&master_addr, sizeof(master_addr));
//
//    master_addr.sin_family = PF_INET;
//    master_addr.sin_port = htons(MASTER_PORT_2);
//    master_addr.sin_addr.s_addr = inet_addr(MASTER_IP);
//
//    if ((cache_master_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
//        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
//        exit(EXIT_FAILURE);
//    }
//    // 连接master
//    if (connect(cache_master_sock, (struct sockaddr *) (&master_addr), sizeof(struct sockaddr)) == -1) {
//        fprintf(stderr, "Connect failed\n");
//        exit(EXIT_FAILURE);
//    }
//
//    while (true) {
//        // 接收信息
//        std::cout << "Receiving message:" << std::endl;
//
//        // 接收主从信息和对应主从cache的IP#port，格式：Primary/Replica#IP#port
//        bzero(recv_buff_master, BUF_SIZE);
//        recv(cache_master_sock, recv_buff_master, BUF_SIZE, 0);
//
//        std::cout << "Receive from master:" << recv_buff_master << std::endl;
//
//    }
//}


//void Cache:: Master_chat(){
//    int cache_master_sock;
//    struct sockaddr_in master_addr;
//    // 向master发送的心跳包
//    char recv_buff_master[BUF_SIZE];
//
//    bzero(&master_addr, sizeof(master_addr));
//
//    master_addr.sin_family = PF_INET;
//    master_addr.sin_port = htons(MASTER_PORT);
//    master_addr.sin_addr.s_addr = inet_addr(MASTER_IP);
//
//    if ((cache_master_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
//        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
//        exit(EXIT_FAILURE);
//    }
//    // 连接master
//    if (connect(cache_master_sock, (struct sockaddr *) (&master_addr), sizeof(struct sockaddr)) == -1) {
//        fprintf(stderr, "Connect failed\n");
//        exit(EXIT_FAILURE);
//    }
//
//    while(true){
//        std::cout << "Receiving message:" << std::endl;
//        bzero(recv_buff_master, BUF_SIZE);
//        recv(cache_master_sock, recv_buff_master, BUF_SIZE, 0);
//        std::cout << "Receive from master:" << recv_buff_master << std::endl;
//        ReadFromMaster(recv_buff_master);
//    }
//}


void Cache::Client_chat() {
    //线程池
    ThreadPool TP(MAX_THREADS_NUMBER);
    std::string buffer;
    std::queue<std::future<int>> future_queue;
    //LRU缓存
    LRU_Cache<std::string, std::string> MainCache(LRU_CAPACITY);
    int serv_client_sock;
    struct sockaddr_in serv_client_addr;
    // 向client发送的数据
    char send_buff_client[BUF_SIZE];
    // client返回的数据
    char recv_buff_client[BUF_SIZE];

    //处理client的需求的任务函数。
    auto LRU_handle_task = [&](std::string message, int targetPort) -> int {
        //分割协议报文
        int spear = 0;
        for (int i = 0; i < message.size(); i++) {
            if (message[i] != '#') {
                ++spear;
            } else {
                break;
            }
        }
        std::cout << "message = " + message << std::endl;
        if (spear == message.size()) {
            //client要求读出值
            std::string key = message;
            std::string res;
            if (MainCache.check(key) > 0) {
                res = MainCache.get(key);
                buffer = "SUCCESS#" + res + "#" + local_cache_IP_ + ":" + port_for_client_;

            } else {
                buffer = "FAILED#" + key + "#" + local_cache_IP_ + ":" + port_for_client_;
            }

            return targetPort;
        } else {
            //client要求写入值
            std::string key = message.substr(0, spear);
            std::string val = message.substr(spear + 1, message.size());
            MainCache.put(key, val);
            buffer = "SUCCESS#" + key + "#" + local_cache_IP_ + ":" + port_for_client_;

            return targetPort;
        }
    };

    auto get_port = [](std::future<int> &future) -> int {
        return future.get();
    };


    bzero(&serv_client_addr, sizeof(serv_client_addr));

    char local_cache_IP[BUF_SIZE];
    memset(local_cache_IP, 0, sizeof(local_cache_IP));
    strcpy(local_cache_IP, local_cache_IP_.c_str());
    int local_cache_port = stoi(port_for_client_);
    serv_client_addr.sin_family = PF_INET;
    serv_client_addr.sin_port = htons(local_cache_port);
    serv_client_addr.sin_addr.s_addr = inet_addr(local_cache_IP);

    if ((serv_client_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }


    int setsockopt_val = 1;
    if (setsockopt(serv_client_sock, SOL_SOCKET, SO_REUSEADDR, &setsockopt_val, sizeof(int)) < 0) {
        std::cout << "setsockopt error" << std::endl;
        exit(1);
    }

    if (bind(serv_client_sock, (struct sockaddr *) &serv_client_addr, sizeof(serv_client_addr)) == -1) {
        fprintf(stderr, "bind error\n");
        exit(EXIT_FAILURE);
    }

    if (listen(serv_client_sock, 5) == -1) {
        fprintf(stderr, "listen error\n");
        exit(EXIT_FAILURE);
    }

    std::cout << "Start to listen client" << std::endl;

    epoll_event client_events[MAX_EVENT_NUMBER];

    // 在内核中创建事件表
    int epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0) {
        perror("epfd error");
        exit(-1);
    }
    // 往事件表中添加监听事件
    addfd(epfd, serv_client_sock, true);
    // 开辟共享内存


    while (true) {
        // epoll_events_count表示就绪事件数目
        int epoll_events_count = epoll_wait(epfd, client_events, EPOLL_SIZE, -1);

        if (epoll_events_count < 0) {
            perror("epoll failure");
            break;
        }

        for (int i = 0; i < epoll_events_count; i++) {
            // 新用户连接
            if (client_events[i].data.fd == serv_client_sock) {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(client_address);
                int client_fd = accept(serv_client_sock, (struct sockaddr *) &client_address, &client_addrLength);
                std::cout << "client connection from:" << inet_ntoa(client_address.sin_addr) << ":"
                          << ntohs(client_address.sin_port) << ", client_fd = " << client_fd << std::endl;

                addfd(epfd, client_fd, true);
            } else {
                bzero(recv_buff_client, BUF_SIZE);
                int len = recv(client_events[i].data.fd, recv_buff_client, BUF_SIZE - 1, 0);
                if (len == 0) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, client_events[i].data.fd, NULL);
                    close(client_events[i].data.fd);
                    std::cout << "close client:" << client_events[i].data.fd << std::endl;
                }
                else {
//                    LRU_handle_task(recv_buff_client, i);
//                    send(client_events[i].data.fd, send_buff_client, BUF_SIZE, 0);
                    auto future = TP.enqueue(LRU_handle_task, recv_buff_client, i);
                    future_queue.emplace(std::move(future));
                    int targetPort = get_port(future_queue.front());
                    memcpy(send_buff_client, buffer.c_str(), buffer.size());
//                    //向端口传出数据：SUCCESS/FAILED#key#ip:port(cache server)
                    send(client_events[targetPort].data.fd, send_buff_client, BUF_SIZE, 0);
                    std::cout << "========================================return value:" << send_buff_client << std::endl;

                    // TODO:1.client关闭后cache被强制关闭，可能需改进send
                    // TODO:2.将最新的状态写入缓冲区，用于Replica
                    //移除事件
                    future_queue.pop();
                }
                bzero(send_buff_client, BUF_SIZE);
            }
        }
    }
    close(serv_client_sock);
    close(epfd);
}


//void Cache::ReadFromMaster(std::string message) {
//    int spear = 2;
//    while (spear != message.size() && message[spear] != '#') {
//        spear++;
//    }
//    std::string head = message.substr(0,1);   //P或者R
//    std::lock_guard<std::mutex> guard(mutex);
//    if(head == "S"){
//        status_ = head;
//        std::string tmp = "";
//        int count = 0;
//        for(int i = 1; i < message.size(); i++){
//            if(message[i] != '#'){
//                tmp += message[i];
//            }else{
//                if(count % 2 == 0){
//                    otherIP.push_back(tmp);
//                }else{
//                    otherPort.push_back(tmp);
//                }
//                count++;
//                tmp.clear();
//            }
//        }
//        otherPort.push_back(tmp);
//    }else if(head =="P"){
//        status_ = head;
//        replica_IP_ = message.substr(2, spear);
//        port_for_replica = message.substr(spear + 1, message.size());
//    }else if(head == "R"){
//        status_ = head;
//        primary_IP_ = message.substr(2, spear);
//        port_for_primary = message.substr(spear + 1, message.size());
//    }
//}


// 注册新的fd到epollfd中
// 参数enable_et表示是否启用ET模式，如果为True则启用，否则使用LT模式
void addfd(int epollfd, int fd, bool enable_et) {
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (enable_et)
        ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    // 设置socket为nonblocking模式
    // 执行完就转向下一条指令，不管函数有没有返回。
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
    printf("fd added to epoll!\n\n");
}
