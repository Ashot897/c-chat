#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT        8080
#define MAX_CLIENTS 10

int main()
{
	int server_fd;
	int new_client;
	int client_sockets[MAX_CLIENTS] = {0};
	struct sockaddr_in address;
	socklen_t addrlen = sizeof(address);
	char buffer[4096];
	fd_set readfds;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == 0)
	{
		perror("Socket failed");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	bind(server_fd, (struct sockaddr *)&address, sizeof(address));
	listen(server_fd, 5);
	printf("Server started of port %d\n", PORT);

	while (1)
	{
		FD_ZERO(&readfds);
		FD_SET(server_fd, &readfds);
		int max_fd = server_fd;
	}

	return 0;
}
