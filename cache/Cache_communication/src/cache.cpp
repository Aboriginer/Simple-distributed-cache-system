//
// Created by Aboriginer on 12/1/2021.
//

#include "cache.h"

Cache::Cache(int cache_size_local) {
    cache_size_local_ = cache_size_local;
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
    int serv_client_sock;
    struct sockaddr_in serv_client_addr;
    // 向client发送的数据
    char send_buff_client[BUF_SIZE];
    // client返回的数据
    char recv_buff_client[BUF_SIZE];

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
                    send(client_events[i].data.fd, send_buff_client, BUF_SIZE, 0);
                    // TODO:写个队列
                    bzero(send_buff_client, BUF_SIZE);
                    // TODO:移除事件
                    // TODO:返回给client的信息ip:port#state#key
                }   else {  // 写请求
                    std::cout << "Write from client to cache LRU:" << recv_buff_client << std::endl;
                    // TODO:LRU
                    std::cout << "write successfully" << std::endl;
                    // TODO:移除事件
                    // TODO:client直接退出会导致cache server退出
                }
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

