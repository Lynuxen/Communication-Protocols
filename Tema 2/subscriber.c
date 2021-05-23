// DIMOVSKI KIRJAN 322CB

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "structures.h"
#include "helpers.h"

void usage(const char *file) {
	fprintf(stderr, "Usage: %s client_id server_address server_port\n", file);
	exit(0);
}

int main(int argc, char const *argv[]) {
	int i, n, ret, sock_fd;

	struct sockaddr_in serv_addr;

	char buffer[BUFLEN];

	fd_set read_fds;
	fd_set tmp_fds;
	int fd_max;

	if (argc < 3) {
		usage(argv[0]);
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sock_fd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sock_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	FD_SET(sock_fd, &read_fds);
	FD_SET(0, &read_fds);
	fd_max = sock_fd;

	int len = strlen(argv[1]);
	strcpy(buffer, argv[1]);
	buffer[len] = ' ';
	buffer[len + 1] = '\0';
	strcat(buffer, argv[3]);
	n = send(sock_fd, buffer, sizeof(buffer), 0);
	DIE(n < 0, "send");

	while (1) {
		tmp_fds = read_fds;

		ret = select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fd_max; ++i) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == 0) {
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);
					buffer[strlen(buffer) - 1] = '\0';
					if (!strcmp(buffer, "exit")) {
						len = strlen(buffer);
						buffer[len] = ' ';
						buffer[len + 1] = '\0';
						strcat(buffer, argv[1]);
						n = send(sock_fd, buffer, sizeof(buffer), 0);
						return 0;
					}
					len = strlen(buffer);
					buffer[len] = ' ';
					buffer[len + 1] = '\0';
					strcat(buffer, argv[1]);
					n = send(sock_fd, buffer, sizeof(buffer), 0);
					DIE(n < 0, "send");
				} else {
					if (recv(sock_fd, buffer, sizeof(buffer), 0) == 0) {
						fprintf(stderr, "Server has crashed!\n");
						exit(-2);
					}
					if (!strcmp(buffer, "exit 1")) {
						fprintf(stderr, "Connection refused: Invalid ID\n");
						return -1;
					} else if (!strncmp(buffer, "exit 0", 6)) {
						fprintf(stderr, "Server has shutdown!\n");
						return -1;
					} else if (!strcmp(buffer, "exit 2")) {
						fprintf(stderr, "ID (%s) is already in use!\n", argv[1]);
						return -1;
					}
					printf("%s\n", buffer);
					memset(buffer, 0, BUFLEN);
				}
			}
		}
	}
	return 0;
}