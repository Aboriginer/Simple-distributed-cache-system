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

    // 以下成员变量用于容灾
    target_IP_ = "None";
    target_port_ = "None";
    cache_list_update_flag = false;
    kv_update_flag = false;
}

//void Cache::Start() {
//
//    auto Heartbeat_bind = std::bind(&Cache::Heartbeat,  this);
//    std::thread for_master_Heartbeat(Heartbeat_bind);
//
//    auto init_bind = std::bind(&Cache::init,  this);
//    std::thread for_init(init_bind);
//    for_init.join();
//
//    if (init_status == 1) {
//        auto Client_chat_bind = std::bind(&Cache::Client_chat, this);
////    auto cache_pass_bind = std::bind(&Cache::cache_pass, this);
//
////    std::thread for_master_chat(Master_chat);
//        std::thread for_client(Client_chat_bind);
////    std::thread for_cache_pass(cache_pass_bind);
////    std::thread for_replica(ToReplica);
//
//
////    for_master_chat.join();
//        for_client.join();
////    for_cache_pass.join();
////    for_replica.join();
//    }
//    for_master_Heartbeat.join();
//}

void Cache::Start() {
    auto Heartbeat_bind = std::bind(&Cache::Heartbeat,  this);
    auto Client_chat_bind = std::bind(&Cache::Client_chat, this);
    auto Cache_pass_bind = std::bind(&Cache::cache_pass, this);
    auto Cache_replica_bind = std::bind(&Cache::replica_chat, this);

    std::thread for_master_Heartbeat(Heartbeat_bind);
    std::thread for_client(Client_chat_bind);
    std::thread for_cache(Cache_pass_bind);
    std::thread for_replica(Cache_replica_bind);



    for_replica.join();
    for_cache.join();
    for_client.join();
    for_master_Heartbeat.join();
}


void Cache::Heartbeat() {
    Timer *timer = new Timer(1000, false, NULL, NULL); //500ms上传一次心跳包
    // 向master发送的心跳包
    char send_buff_master[BUF_SIZE];

    std::string master_port = std::to_string(MASTER_PORT);
    int cache_master_sock = client_socket(MASTER_IP, master_port);

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


void Cache::Client_chat() {
    //线程池
    ThreadPool TP(MAX_THREADS_NUMBER);
    std::string buffer;
    std::queue<std::future<int>> future_queue;
    //LRU缓存
//    LRU_Cache<std::string, std::string> MainCache(LRU_CAPACITY);

    int serv_client_sock = server_socket(local_cache_IP_, port_for_client_);
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


//向目标IP传递信息
void Cache::to_single_cache(std::string &ip, std::string &port, std::string &key){
    int cache_cache_sock;
    // int
    struct sockaddr_in master_addr;
    char send_buff_master[BUF_SIZE];
    // Master返回的扩缩容信息
    // char recv_buff_master[BUF_SIZE];
    master_addr.sin_family = PF_INET;
    int int_port = stoi(port);
    master_addr.sin_port = htons(int_port);
    master_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if ((cache_cache_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // 连接master
    if (connect(cache_cache_sock, (struct sockaddr *) (&master_addr), sizeof(struct sockaddr)) == -1) {
        fprintf(stderr, "Connect failed\n");
        exit(EXIT_FAILURE);
    }

    bzero(send_buff_master, BUF_SIZE);
    std::string val = MainCache.get(key);
    std::string message = key + "#" + val;
    strcpy(send_buff_master, message.data());
    std::cout<<"sending message = "<<send_buff_master<<std::endl;
    send(cache_cache_sock, send_buff_master, BUF_SIZE, 0);
    std::cout<<"message send."<<std::endl;
    bzero(send_buff_master, BUF_SIZE);
}

void slice(std::string message, std::string &key, std::string &val){
    int spear = 0;
    while(spear != message.size() && message[spear] != '#'){
        spear ++;
    }
    key = message.substr(0, spear);
    val = message.substr(spear + 1, message.size());
}


//从目标IP地址接受信息
void Cache::from_single_cache(std::string &ip, std::string &port){
    int cache_cache_sock;
    // int
    struct sockaddr_in master_addr;
    char recv_buff_master[BUF_SIZE];
    // Master返回的扩缩容信息
    // char recv_buff_master[BUF_SIZE];
    master_addr.sin_family = PF_INET;
    int int_port = stoi(port);
    master_addr.sin_port = htons(int_port);
    master_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if ((cache_cache_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // 连接master
    socklen_t size = sizeof(struct sockaddr);
    if (accept(cache_cache_sock,(struct sockaddr *)(&master_addr), &size) == -1) {
        fprintf(stderr, "Connect failed\n");
        exit(EXIT_FAILURE);
    }


    recv(cache_cache_sock, recv_buff_master, BUF_SIZE, 0);
    std::cout<<"receving message = "<<recv_buff_master<<std::endl;
    std::string key ,val;
    slice(recv_buff_master, key ,val);
    MainCache.put(key, val);
    std::cout<<"receved."<<std::endl;

}

void Cache:: Master_chat(){
    int cache_master_sock;
    struct sockaddr_in master_addr;
    // 向master发送的心跳包
    char recv_buff_master[BUF_SIZE];

    bzero(&master_addr, sizeof(master_addr));

    master_addr.sin_family = PF_INET;
    master_addr.sin_port = htons(MASTER_PORT);
    master_addr.sin_addr.s_addr = inet_addr(MASTER_IP);

    if ((cache_master_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    // 连接master
    if (connect(cache_master_sock, (struct sockaddr *) (&master_addr), sizeof(struct sockaddr)) == -1) {
        fprintf(stderr, "Connect failed\n");
        exit(EXIT_FAILURE);
    }

    while(true){
        std::cout << "Receiving message:" << std::endl;
        bzero(recv_buff_master, BUF_SIZE);
        recv(cache_master_sock, recv_buff_master, BUF_SIZE, 0);
        std::cout << "Receive from master:" << recv_buff_master << std::endl;
        ReadFromMaster(recv_buff_master);
    }
}

void Cache::ReadFromMaster(std::string message) {
    int spear = 2;
    while (spear != message.size() && message[spear] != '#') {
        spear++;
    }
    std::string head = message.substr(0,1);   //P或者R
    std::lock_guard<std::mutex> guard(mutex);
    if(head == "S"){
        status_ = head;
        std::string tmp = "";
        int count = 0;
        for(int i = 1; i < message.size(); i++){
            if(message[i] != '#'){
                tmp += message[i];
            }else{
                if(count % 2 == 0){
                    otherIP.push_back(tmp);
                }else{
                    otherPort.push_back(tmp);
                }
                count++;
                tmp.clear();
            }
        }
        otherPort.push_back(tmp);
    }else if(head =="P"){
        status_ = head;
        replica_IP_ = message.substr(2, spear);
        port_for_replica = message.substr(spear + 1, message.size());
    }else if(head == "R"){
        status_ = head;
        primary_IP_ = message.substr(2, spear);
        port_for_primary = message.substr(spear + 1, message.size());
    }


}

//扩缩容函数
void Cache::cache_pass(){
    std::cout <<"select status"<<std::endl;
    std::cin >> status_;
    otherIP.push_back("127.0.0.1");
    if(port_for_cache_ == "8888"){
        otherPort.push_back("8889");
    }else{
        otherPort.push_back("8888");
    }
    if(status_ == "S"){
        if(otherIP.size() == 0){
            std::cout<<"IP address is not sent by the master."<<std::endl;
            return;
        }
        int size = otherIP.size();
        for(int i = 0; i < otherIP.size(); i++){
            to_single_cache(otherIP[i], otherPort[i], out_key[i]);
        }
    }else if(status_ == "P"){
        dying_cache_IP_ = "127.0.0.1";
        if(port_for_cache_ == "8888"){
            dying_cache_Port = "8889";
        }else{
            dying_cache_Port = "8888";
        }
        from_single_cache(dying_cache_IP_, dying_cache_Port);
    }
}

//更新其他IP地址
void Cache::update_cache(std::string &IP, std::string &port,std::string status){
    if(status == "N"){
        other_Cache[IP] = port;
    }else if(status == "K"){
        dying_cache_IP_ = IP;
        dying_cache_Port = port;
        other_Cache.erase(IP);
    }
}


// 容灾
void Cache::replica_chat() {
    while (true) {
        if (status_ == "P") {   // 本地cache是主cache，作为服务器端
            target_port_ = "hhhh";  // TODO:为测试replica_chat, 需删除
            if (target_port_ != "None") {    // 确定备份cache已上线

                int clnt_sock;
                struct sockaddr_in clnt_adr;
                socklen_t clnt_adr_sz = sizeof(clnt_adr);
                int serv_sock = server_socket(local_cache_IP_, port_for_cache_);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
                std::cout << "client connection from:" << inet_ntoa(clnt_adr.sin_addr) << ":"
                          << ntohs(clnt_adr.sin_port) << ", client_fd = " << clnt_sock << std::endl;

                std::string send_message;
                char send_buff_replica[BUF_SIZE];
                while (true) {
                    cache_list_update_flag = true;  // 用于测试
                    kv_update_flag = true;  // 用于测试
                    if (cache_list_update_flag) {   // cache_list有更新
                        std::cout << "Need to write cache_list" << std::endl;
                        send_message.clear();
                        bzero(send_buff_replica, BUF_SIZE);
                        // TODO:写入更新的cache_list
                        send_message = "#"; // 第一个字符是'#', 则表示收到的是cache_list
                        send_message += "cache_list";
                        std::cout << "Send cache_list to replica cache, fd = " << clnt_sock << std::endl;
                        strcpy(send_buff_replica, send_message.data());
                        send(clnt_sock, send_buff_replica, BUF_SIZE, 0);
                        std::cout << "Send message:" << send_buff_replica << std::endl;
                        cache_list_update_flag = false; // 成功写入cache_list
                    }
                    if (kv_update_flag) {
                        std::cout << "Need to write key/key#value" << std::endl;
                        send_message.clear();
                        bzero(send_buff_replica, BUF_SIZE);
                        // TODO:写入更新的key/key#value
                        send_message = "key#value";
                        std::cout << "Send key/key#value to replica cache, fd = " << clnt_sock << std::endl;
                        strcpy(send_buff_replica, send_message.data());
                        send(clnt_sock, send_buff_replica, BUF_SIZE, 0);
                        std::cout << "Send message:" << send_buff_replica << std::endl;
                        kv_update_flag = false; // 成功写入cache_list
                    }
                    sleep(3);   // TODO:测试用，需删除
                }
                close(clnt_sock);
            }
        }   else {  // 本地cache是备份cache，作为客户端

            std::string recv_message;
            char recv_buff_primary[BUF_SIZE];
            // TODO:这里只是为了测试，后面需删除
            target_IP_ = "127.0.0.1";
            target_port_ = "8888";
            std::cout << "Server connection from:";
            int clnt_sock = client_socket(target_IP_, target_port_);
            while (true) {
                recv_message.clear();
                bzero(recv_buff_primary, BUF_SIZE);
                int len = recv(clnt_sock, recv_buff_primary, BUF_SIZE, 0);
                recv_message = std::string(recv_buff_primary);
                std::cout << "Receive from primary cache:" << recv_message << std::endl;
                // TODO:解析收到的recv_message
                if (recv_message[0] == '#') {   // 收到更新的cache_list
                    std::cout << "Receive cache_list" << std::endl;
                }
                else{   // 收到更新的key/key#value
                    std::cout << "Receive key/key#value" << std::endl;
                }
            }
            close(clnt_sock);
        }
    }
}


 /*在调用server_socket函数前写入以下内容即可：
    int clnt_sock;
    struct sockaddr_in clnt_adr;
    socklen_t clnt_adr_sz;

    clnt_adr_sz = sizeof(clnt_adr);

    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
    std::cout << "client connection from:" << inet_ntoa(clnt_adr.sin_addr) << ":"
              << ntohs(clnt_adr.sin_port) << ", client_fd = " << clnt_sock << std::endl;
 */
int server_socket(std::string server_IP, std::string server_port) {
    int serv_sock;
    struct sockaddr_in serv_adr;

    if ((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {  // SOCK_STREAM表示这个套接字是一个连接的端点
        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    char server_IP_[BUF_SIZE];
    memset(server_IP_, 0, sizeof(server_IP_));
    strcpy(server_IP_, server_IP.c_str());
    bzero(&serv_adr, sizeof(serv_adr));
    serv_adr.sin_family = PF_INET;
    serv_adr.sin_port = htons(stoi(server_port));
    serv_adr.sin_addr.s_addr = inet_addr(server_IP_);

    int setsockopt_val = 1;
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &setsockopt_val, sizeof(int)) < 0) {
        std::cout << "setsockopt error" << std::endl;
        exit(1);
    }

    if (bind(serv_sock, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) == -1) {
        fprintf(stderr, "bind error\n");
        exit(EXIT_FAILURE);
    }

    if (listen(serv_sock, 5) == -1) {
        fprintf(stderr, "listen error\n");
        exit(EXIT_FAILURE);
    }

    std::cout << "Start to listen client" << std::endl;

    return serv_sock;
}

int client_socket(std::string server_IP, std::string server_port) {
    int sock;
    int str_len;

    struct sockaddr_in serv_adr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    char server_IP_[BUF_SIZE];
    memset(server_IP_, 0, sizeof(server_IP_));
    strcpy(server_IP_, server_IP.c_str());
    bzero(&serv_adr, sizeof(serv_adr));
    serv_adr.sin_family = PF_INET;
    serv_adr.sin_addr.s_addr=inet_addr(server_IP_);
    serv_adr.sin_port = htons(stoi(server_port));

    // 因为调用了connect，所以sock是客户端套接字
    //connect函数返回后，并不立刻进行数据交换（只是服务器端把来连接请求信息记录到等待队列）
    if (connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1){
        fprintf(stderr, "Connect failed\n");
        exit(EXIT_FAILURE);
    }
    else
        puts("Connected........");

    return sock;
}


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
