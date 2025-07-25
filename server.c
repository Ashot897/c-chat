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
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			int fd = client_sockets[i];
			if (fd > 0)
			{
				FD_SET(fd, &readfds);
			}
			if (fd > max_fd)
				max_fd = fd;
		}
		select(max_fd + 1, &readfds, NULL, NULL, NULL);
		if (FD_ISSET(server_fd, &readfds))
		{
			new_client = accept(server_fd, (struct sockaddr *)&address, &addrlen);
			printf("New connection!\n Socket FD: %d\n IP: %s\n", new_client, inet_ntoa(address.sin_addr));

			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (client_sockets[i] == 0)
				{
					client_sockets[i] = new_client;
					break;
				}
			}
		}
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			int fd = client_sockets[i];
			if (FD_ISSET(fd, &readfds))
			{
				int val = read(fd, buffer, 4096);
				if (val == 0)
				{
					getpeername(fd, (struct sockaddr *)&address, &addrlen);
					printf("Client disconnetted. IP: %s\n", inet_ntoa(address.sin_addr));
					close(fd);
					client_sockets[i] = 0;
				}
				else
				{
					buffer[val] = '\0';
					for (int j = 0; j < MAX_CLIENTS; j++)
					{
						if (client_sockets[j] != 0 && client_sockets[j] != fd)
						{
							send(client_sockets[j], buffer, strlen(buffer), 0);
						}
					}
				}
			}
		}
	}

	return 0;
}














