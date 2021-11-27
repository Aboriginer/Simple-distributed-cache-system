#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "RandString.hpp"

#define BUF_SIZE 0xFFFF
#define CACHE_SEVER_PORT 8887

int main(int argc, char *argv[])
{
    struct sockaddr_in cache_sever_addr;

    cache_sever_addr.sin_family = PF_INET;
    cache_sever_addr.sin_port = htons(CACHE_SEVER_PORT);
    cache_sever_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int listener = 0;
    listener = socket(PF_INET, SOCK_STREAM, 0);
    if (listener < 0)
    {
        perror("listener");
        exit(-1);
    }

    if (bind(listener, (struct sockaddr *)&cache_sever_addr, sizeof(cache_sever_addr)) < 0)
    {
        perror("bind error");
        exit(-1);
    }

    int ret = listen(listener, 5);
    if (ret < 0)
    {
        perror("listen error");
        exit(-1);
    }

    struct sockaddr_in client_address;
    socklen_t client_addrLength = sizeof(struct sockaddr_in);

    char recv_buff[BUF_SIZE], send_buff[BUF_SIZE];
    bzero(recv_buff, BUF_SIZE); //key
    bzero(send_buff, BUF_SIZE); //value

    while (true)
    {
        int clientfd = accept(listener,
                              (struct sockaddr *)&client_address,
                              &client_addrLength);
        std::cout << "client connection from: "
                  << inet_ntoa(client_address.sin_addr) << ":"
                  << ntohs(client_address.sin_port) << ", clientfd = "
                  << clientfd << std::endl;
        int len = recv(clientfd, recv_buff, BUF_SIZE, 0);

        std::string return_value = strRand(10);

        std::cout << "Key: " << recv_buff << "   Return Value: "
                  << return_value << std::endl;

        //无论读写均返回随机字符串，仅作测试通信用
        strcpy(send_buff, return_value.data());
        send(clientfd, send_buff, BUF_SIZE, 0);

        memset(recv_buff, 0, sizeof(recv_buff));
        memset(send_buff, 0, sizeof(recv_buff));
        
        close(clientfd);
    }
}
