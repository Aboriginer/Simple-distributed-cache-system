//
// Created by Aboriginer on 12/3/2021.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include  <sys/socket.h>

#define BUF_SIZE 1024
#define CACHE_SERVER_IP "127.0.0.1"

void error_handling(char *message);



int main(int argc, char *argv[]) {
    int sock;
    char message[BUF_SIZE];
    int str_len;

    struct sockaddr_in serv_adr;

    if (argc != 2) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // 这里的sock还没定下来是服务端套接字还是客户端套接字
    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr=inet_addr(CACHE_SERVER_IP);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        std::cout << "connect error" << std::endl;
    else
        puts("Connected........");

    while (1) {
        fputs("Input message(Q to quit): ", stdout);

        std::cin.getline(message, BUF_SIZE);

        if (!strcmp(message, "q") || !strcmp(message, "Q"))
            break;

        write(sock, message, strlen(message));

//        read(sock, message, BUF_SIZE - 1);
//        printf("Message from server: %s", message);
    }
    close(sock);

    return 0;
}


