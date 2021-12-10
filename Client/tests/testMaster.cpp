#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

#define BUF_SIZE 0xFFFF
#define MASTER_PORT 8889

int main(int argc, char *argv[])
{
	struct sockaddr_in master_addr;

	master_addr.sin_family = PF_INET;
	master_addr.sin_port = htons(MASTER_PORT);
	master_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int listener = 0;
	listener = socket(PF_INET, SOCK_STREAM, 0);
	if (listener < 0)
	{
		perror("listener");
		exit(-1);
	}

	if (bind(listener, (struct sockaddr *)&master_addr, sizeof(master_addr)) < 0)
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
	int clientfd = accept(listener,
												(struct sockaddr *)&client_address,
												&client_addrLength);

	std::cout << "client connection from: "
						<< inet_ntoa(client_address.sin_addr) << ":"
						<< ntohs(client_address.sin_port) << ", clientfd = "
						<< clientfd << std::endl;

	char recv_buff[BUF_SIZE], send_buff[BUF_SIZE];
	bzero(recv_buff, BUF_SIZE);
	bzero(send_buff, BUF_SIZE);

	while (true)
	{
		std::cout << "Request from client, fd = " << clientfd << std::endl;

		int len = recv(clientfd, recv_buff, BUF_SIZE, 0);

		std::cout << "Key: " << recv_buff << "   Return CacheSever addr: "
							<< "127.0.0.1:8887" << std::endl;
		std::string temp = recv_buff;

		// message example: SUCCESS/FAILED#key#ip:port
		strcpy(send_buff, ("SUCCESS#" + temp + "#127.0.0.1:8887").data()); //CacheSever IP
		send(clientfd, send_buff, BUF_SIZE, 0);

		memset(recv_buff, 0, sizeof(recv_buff));
		memset(send_buff, 0, sizeof(recv_buff));
	}
	close(clientfd);
}
