#include <iostream>

#include "Newcli.h"

using namespace std;

// 客户端类成员函数

// 客户端类构造函数
nClient::nClient(uint16_t port){
    
    // 初始化要连接的服务器地址和端口
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    port = port;
    // 初始化socket
    sock = 0;
    
    // 初始化进程号
    pid = 0;
    
    // 客户端状态
    isClientwork = true;
    
    // epool fd
    epfd = 0;
}

// 连接服务器
void nClient::Connect() {
    cout << "Connect Server: " << SERVER_IP << " : " << this->port << endl;
    
    // 创建socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        perror("sock error");
        exit(-1); 
    }

    // 连接服务端
    if(connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("connect error");
        exit(-1);
    }

    // 创建管道，其中fd[0]用于父进程读，fd[1]用于子进程写
    if(pipe(pipe_fd) < 0) {
        perror("pipe error");
        exit(-1);
    }

    // 创建epoll
    epfd = epoll_create(EPOLL_SIZE);
    
    if(epfd < 0) {
        perror("epfd error");
        exit(-1); 
    }

    //将sock和管道读端描述符都添加到内核事件表中
    addfd(epfd, sock, true);
    addfd(epfd, pipe_fd[0], true);

}

// 断开连接，清理并关闭文件描述符
void nClient::Close() {

    if(pid){
       //关闭父进程的管道和sock
        close(pipe_fd[0]);
        close(sock);
    }else{
        //关闭子进程的管道
        close(pipe_fd[1]);
    }
}

// 启动客户端
void nClient::Start() {

    // epoll 事件队列
    static struct epoll_event events[2];
    
    // 连接服务器
    Connect();
    
    // 创建子进程
    pid = fork();
    
    // 如果创建子进程失败则退出
    if(pid < 0) {
        perror("fork error");
        close(sock);
        exit(-1);
    } else if(pid == 0) {
        // 进入子进程执行流程
        //子进程负责写入管道，因此先关闭读端
        close(pipe_fd[0]); 

        // 输入exit可以退出聊天室
        cout << "Please input 'exit' to exit the chat room" << endl;

        // 如果客户端运行正常则不断读取输入发送给服务端
        while(isClientwork){
            bzero(&message, BUF_SIZE);
            fgets(message, BUF_SIZE, stdin);

            // 客户输出exit,退出
            if(strncasecmp(message, EXIT, strlen(EXIT)) == 0){
                isClientwork = 0;
            }
            // 子进程将信息写入管道
            else {
                if( write(pipe_fd[1], message, strlen(message) - 1 ) < 0 ) { 
                    perror("fork error");
                    exit(-1);
                }
            }
        }
    } else { 
        //pid > 0 父进程
        //父进程负责读管道数据，因此先关闭写端
        close(pipe_fd[1]); 

        // 主循环(epoll_wait)
        while(isClientwork) {
            int epoll_events_count = epoll_wait( epfd, events, 2, -1 );

            //处理就绪事件
            for(int i = 0; i < epoll_events_count ; ++i)
            {
                bzero(&message, BUF_SIZE);

                //服务端发来消息
                if(events[i].data.fd == sock)
                {
                    //接受服务端消息
                    int ret = recv(sock, message, BUF_SIZE, 0);

                    // ret= 0 服务端关闭
                    if(ret == 0) {
                        cout << "Server closed connection: " << sock << endl;
                        close(sock);
                        isClientwork = 0;
                    } else {
                        cout << message << endl;
                    }
                }
                //子进程写入事件发生，父进程处理并发送服务端
                else { 
                    //父进程从管道中读取数据
                    int ret = read(events[i].data.fd, message, BUF_SIZE);

                    // ret = 0
                    if(ret == 0)
                        isClientwork = 0;
                    else {
                        // 将信息发送给服务端
                        send(sock, message, BUF_SIZE, 0);
                    }
                }
            }//for
        }//while
    }
    
    // 退出进程
    Close();
}