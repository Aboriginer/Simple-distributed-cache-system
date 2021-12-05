//
// Created by Aboriginer on 12/1/2021.
//

#include "cache.h"

Cache::Cache(int cache_size_local) {
    cache_size_local_ = cache_size_local;
    message_to_client = "";
}

void Cache::Start() {
    std::thread for_master(Heartbeat);
    std::thread for_client(Client_chat);

    for_master.join();
    for_client.join();
}

void Cache::Heartbeat() {
    int cache_master_sock;
    struct sockaddr_in master_addr;
    // 向master发送的心跳包
    char send_buff_master[BUF_SIZE];
    // Master返回的扩缩容信息
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
    if (connect(cache_master_sock,(struct sockaddr *)(&master_addr), sizeof(struct sockaddr)) == -1) {
        fprintf(stderr, "Connect failed\n");
        exit(EXIT_FAILURE);
    }

    while (true) {
        // TODO: master经常会接收不到send_buff_master的信息
        std::string heart_message = "I am alive";
        strcpy(send_buff_master, heart_message.data());
        std::cout << "Start send heartbeat" << std::endl;
        std::cout << "Send message:" << send_buff_master << std::endl;

        send(cache_master_sock, send_buff_master, BUF_SIZE, 0);

        std::cout << "Send heartbeat ok" << std::endl;
        bzero(send_buff_master, BUF_SIZE);
        sleep(3);
    }
    close(cache_master_sock);
}

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
    auto LRU_handle_task = [&](std::string message, int targetPort) ->int {
        //分割协议报文
        std::vector<std::string> tmp = split(message, "#");
        
        if(tmp.size() == 1){
            //client要求读出值
            std::string key = tmp[0];
            std::string res;
            res = MainCache.get(key);
            buffer = res;
            return targetPort;
        }else if(tmp.size() > 1){
            //client要求写入值
            MainCache.put(tmp[0],tmp[1]);
            buffer = "write success";
            return targetPort;
        }else{
            //空的请求
            buffer = "empty request.";
            return targetPort;
        }
    };

    bzero(&serv_client_addr, sizeof(serv_client_addr));

    serv_client_addr.sin_family = PF_INET;
    serv_client_addr.sin_port = htons(SERV_CLIENT_CACHE_SEVER_PORT);
    serv_client_addr.sin_addr.s_addr = inet_addr(CACHE_SERVER_IP);

    if ((serv_client_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (bind(serv_client_sock, (struct sockaddr*)&serv_client_addr, sizeof(serv_client_addr)) == -1) {
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

        if(epoll_events_count < 0) {
            perror("epoll failure");
            break;
        }

        for (int i = 0; i < epoll_events_count; i ++ ) {
            // 新用户连接
            if (client_events[i].data.fd == serv_client_sock) {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(client_address);
                int client_fd = accept(serv_client_sock, (struct sockaddr*)&client_address, &client_addrLength);
                std::cout << "client connection from:" << inet_ntoa(client_address.sin_addr) << ":"
                          <<  ntohs(client_address.sin_port) << ", client_fd = " << client_fd << std::endl;

                addfd(epfd, client_fd, true);
            }
            else {
                bzero(recv_buff_client, BUF_SIZE);
                recv(client_events[i].data.fd, recv_buff_client, BUF_SIZE, 0);
                // 判断读请求还是写请求
                char *pch = strchr(recv_buff_client, '#');
                if (pch == nullptr) { // 读请求
                    // TODO:LRU
                    std::cout << "Read from cache LRU to client:" << send_buff_client << std::endl;
                    
                    int targetPort = future_queue.front().get();
                    memcpy(send_buff_client, &buffer, buffer.size());
                    //由于任务队列是异步执行的，所以不确定谁的请求会被首先解决出来。
                    //因此采用了记录targetPort的方法来让请求和答复一一对应。
                    send(client_events[targetPort].data.fd, send_buff_client, BUF_SIZE, 0);
                    // --TODO:写个队列
                    bzero(send_buff_client, BUF_SIZE);
                    future_queue.pop();
                    // TODO:移除事件
                    // TODO:返回给client的信息ip:port#state#key
                }   else {  // 写请求
                    std::cout << "Write from client to cache LRU:" << recv_buff_client << std::endl;
                    // TODO:LRU
                    //?? : 在这里我们可能需要事先分辨client的请求究竟是读还是写。
                    auto future = TP.enqueue(LRU_handle_task, recv_buff_client, i);
                    std::cout << "write successfully" << std::endl;
                    future_queue.emplace(future);
                    // TODO:移除事件
                    // TODO:client直接退出会导致cache server退出
                    // 在这里，我的建议是，让client推出前发送一个“告别信息”，通知cache把对应的fd关掉。
                }

                //处理请求并通过端口进行回复
                //函数将会返回发出请求的客户端主机号
                
                //TODO : 在本地调试下似乎出现了线程竞跑的问题，尚不明确其中原因。
            }
        }
    }
}

void Cache::Cache_server_chat() {
    // TODO
}


// 注册新的fd到epollfd中
// 参数enable_et表示是否启用ET模式，如果为True则启用，否则使用LT模式
void addfd( int epollfd, int fd, bool enable_et )
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if( enable_et )
        ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    // 设置socket为nonblocking模式
    // 执行完就转向下一条指令，不管函数有没有返回。
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0)| O_NONBLOCK);
    printf("fd added to epoll!\n\n");
}

