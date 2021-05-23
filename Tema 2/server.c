// DIMOVSKI KIRJAN 322CB

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <math.h>
#include "helpers.h"
#include "structures.h"

#ifndef INITIAL_CAP
#define INITIAL_CAP 10
#endif

void usage(const char *file) {
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

// initializes the database
TClients *initMem() {
	TClients *new = (TClients *) malloc(sizeof(TClients));
	if (new == NULL) {
		exit(-2);
	}
	new->nmb_cli = 0;
	new->capacity = INITIAL_CAP;
	new->list_cli = NULL;
	return new;
}

// allocates space for a client.
void addClient(TClients *cli, char *ID, int sock) {
	if (cli->nmb_cli == 0) {
		cli->list_cli = (TClient *) malloc(cli->capacity * sizeof(TClient));
	} else if (cli->nmb_cli == cli->capacity) {
		cli->capacity *= 10;
		cli->list_cli = (TClient *) realloc(cli->list_cli, cli->capacity * sizeof(TClient));
	}
	if (cli->list_cli == NULL) {
		exit(-2);
	}
	strcpy(cli->list_cli[cli->nmb_cli].ID, ID);
	cli->list_cli[cli->nmb_cli].sock = sock;
	cli->list_cli[cli->nmb_cli].connect = 1;
	cli->list_cli[cli->nmb_cli].b_capacity = INITIAL_CAP;
	cli->list_cli[cli->nmb_cli].t_capacity = INITIAL_CAP;
	cli->list_cli[cli->nmb_cli].nmb_topics = 0;
	cli->list_cli[cli->nmb_cli].nmb_blog = 0;
	cli->list_cli[cli->nmb_cli].b_log = NULL;
	cli->nmb_cli++;
}

// allocates for a subscription.
TTopic *alocSub(TClient *cli, char *ID, char *topic, int sf) {
	if (cli->nmb_topics == 0) {
		cli->topics = (TTopic *) malloc(cli->t_capacity * sizeof(TTopic));
	} else if (cli->nmb_topics == cli->t_capacity) {
		cli->t_capacity *= 10;
		cli->topics = (TTopic *) realloc(cli->topics, cli->t_capacity * sizeof(TTopic));
	}
	cli->topics[cli->nmb_topics].sf_flag = sf;
	strcpy(cli->topics[cli->nmb_topics].topic, topic);
	cli->nmb_topics++;
	return cli->topics;
}

int isSubscribed(TClient *cli, char *topic, int sf) {
	for (int i = 0; i < cli->nmb_topics; ++i) {
		if (!strcmp(cli->topics[i].topic, topic)) {
			printf("Client %s is already subscribed to %s!\n", cli->ID, topic);
			printf("Changing settings...\n");
			cli->topics[i].sf_flag = sf;
			return 1;
		}
	}
	return 0;
}



void subscribe(TClients *cli, char *ID, char *topic, int sf) {
	int found = 0;

	for (int i = 0; i < cli->nmb_cli; ++i) {
		if (!strcmp(cli->list_cli[i].ID, ID)) {
			if (isSubscribed(&cli->list_cli[i], topic, sf)) {
				return;
			}
			cli->list_cli[i].topics = alocSub(&cli->list_cli[i], ID, topic, sf);
			found = 1;
		}
	}
	if (found == 0) {
		fprintf(stderr, "Client %s not found!\n", ID);
	}
}

void unsubscribe(TClients *cli, char *ID, char *topic) {
	for (int i = 0; i < cli->nmb_cli; ++i) {
		if (!strcmp(cli->list_cli[i].ID, ID)) {
			for (int j = 0; j < cli->list_cli[i].nmb_topics; ++j) {
				if (!strcmp(cli->list_cli[i].topics[j].topic, topic)) {
					for (int c = j; c < cli->list_cli[i].nmb_topics; ++c) {
						cli->list_cli[i].topics[c] = cli->list_cli[i].topics[c + 1];
					}
					cli->list_cli[i].nmb_topics -= 1;
					return;
				}
			}
		}
	}
	return;
}

int checkConnected(TClients *cli, char *ID) {
	for (int i = 0; i < cli->nmb_cli; ++i) {
		if (!strcmp(cli->list_cli[i].ID, ID)) {
			if (cli->list_cli[i].connect == 1) {
				return 1;
			}
		}
	}
	return 0;
}

int checkSubscription(char *buf) {
	char buf_tmp[BUFLEN], *token;
	memset(buf_tmp, 0, BUFLEN);
	strcpy(buf_tmp, buf);

	token = strtok(buf_tmp, " ");
	if (strlen(token) > 10) {
		fprintf(stderr, "ID (%s) is longer than maximum allowed!\n", token);
		return -1;
	}
	return 1;
}

int existingClient(TClients *cli, char *ID, int sock) {
	for (int i = 0; i < cli->nmb_cli; ++i) {
		if (!strcmp(cli->list_cli[i].ID, ID)) {
			return 1;
		}
	}
	return 0;
}

// sets the flags to disconnected, i.e. 0;
void deconnect(TClients *cli, char *ID) {
	for (int i = 0; i < cli->nmb_cli; ++i) {
		if (!strcmp(cli->list_cli[i].ID, ID)) {
			cli->list_cli[i].sock = 0;
			cli->list_cli[i].connect = 0;
		}
	}
}
// sets the flags to connected, i.e. 1;
int reconnect(TClients *cli, char *ID, int socket) {
	for (int i = 0; i < cli->nmb_cli; ++i) {
		if (!strcmp(cli->list_cli[i].ID, ID)) {
			cli->list_cli[i].connect = 1;
			cli->list_cli[i].sock = socket;
			return 1;
		}
	}
	return 0;
}

// allocates memory for the backlog
char **alocMsg(TClient *cli, char *new) {
	if (cli->nmb_blog == 0) {
		cli->b_log = (char **) malloc(cli->b_capacity * sizeof(char *));
		for (int i = 0; i < cli->b_capacity; ++i) {
			cli->b_log[i] = (char *) malloc(BUFLEN * sizeof(char));
		}
	} else if (cli->nmb_blog == cli->b_capacity) {
		cli->b_capacity *= 10;
		cli->b_log = (char **) realloc(cli->b_log , cli->b_capacity * sizeof(char *));
		for (int i = cli->nmb_blog; i < cli->b_capacity; ++i) {
			cli->b_log[i] = (char *) malloc(BUFLEN * sizeof(char));
		}
	}
	memcpy(cli->b_log[cli->nmb_blog], new, sizeof(char) * BUFLEN);
	cli->nmb_blog++;
	return cli->b_log;
}
// sends the backlog
// used when an existing client reconnects
void sendBacklog(TClients cli, char *ID) {
	int n;
	char buffer[BUFLEN];
	memset(buffer, 0, BUFLEN);
	for (int i = 0; i < cli.nmb_cli; ++i) {
		if (cli.list_cli[i].nmb_blog > 0 && !strcmp(cli.list_cli[i].ID, ID)) {
			for (int j = 0; j < cli.list_cli[i].nmb_blog; ++j) {
				strcpy(buffer, cli.list_cli[i].b_log[j]);
				free(cli.list_cli[i].b_log[j]);
				n = send(cli.list_cli[i].sock, buffer, sizeof(buffer), 0);
				DIE(n < 0, "send");
			}
			free(cli.list_cli[i].b_log);
			cli.list_cli[i].nmb_blog = 0;
			return;
		}
	}
}
// will send the UDP message to clients who are subscribed and connected
// otherwise, updates the backlog.
void sendUDP(TClients *cli, char *new, TMsg tmp_msg) {
	int n;
	char buffer[BUFLEN];
	memset(buffer, 0, BUFLEN);
	strcpy(buffer, new);
	for (int i = 0; i < cli->nmb_cli; ++i) {
		for (int j = 0; j < cli->list_cli[i].nmb_topics; ++j) {
			if (!strcmp(cli->list_cli[i].topics[j].topic, tmp_msg.topic)) {
				if (cli->list_cli[i].connect == 1) {
					n = send(cli->list_cli[i].sock, buffer, sizeof(buffer), 0);
					DIE(n < 0, "send");
					break;
				} else if (cli->list_cli[i].topics[j].sf_flag == 1) {
					cli->list_cli[i].b_log = alocMsg(&cli->list_cli[i], new);
					break;
				}
			}
		}
	}
}

// parse the operation from the client
Toperation *parseOperation(char *buffer) {
	char *token, tmp[BUFLEN];
	int i = 0;
	strcpy(tmp, buffer);

	Toperation *tmp_op = (Toperation *) malloc(sizeof(Toperation));
	tmp_op->err_flag = 0;
	tmp_op->err_topic = 0;
	token = strtok(tmp, " ");
	while (token != NULL) {
		if (i == 0) {
			if (!strcmp(token, "exit") || !strcmp(token, "unsubscribe") ||
			        !strcmp(token, "subscribe")) {
				strcpy(tmp_op->operation, token);
			} else {
				strcpy(tmp_op->operation, "error");
				break;
			}
		} else if (i == 1) {
			if (!strcmp(tmp_op->operation, "exit")) {
				strcpy(tmp_op->ID, token);
				break;
			} else {
				strcpy(tmp_op->topic, token);
				if (strlen(tmp_op->topic) > 50) {
					tmp_op->err_topic = 1;
				}
			}
		} else if (i == 2) {
			if (!strcmp(tmp_op->operation, "unsubscribe")) {
				strcpy(tmp_op->ID, token);
				break;
			}
			tmp_op->sf_flag = atoi(token);
			if (tmp_op->sf_flag == 0 || tmp_op->sf_flag == 1) {
				tmp_op->err_flag = 0;
			} else {
				tmp_op->err_flag = 1;
			}
		} else if (i == 3) {
			strcpy(tmp_op->ID, token);
		}
		++i;
		token = strtok(NULL, " ");
	}
	return tmp_op;
}
// used for debugging purpuses.
void printBacklog(TClient cli) {
	for (int i = 0; i < cli.nmb_blog; ++i) {
		printf("%s\n", cli.b_log[i]);
	}
}

void printClient(TClient cli) {
	printf("Client %s, connected: %d, socket: %d\n",
	       cli.ID, cli.connect, cli.sock);
}

void printSubscriptions(TClient cli) {
	printf("Subscriptions: \n");
	for (int i = 0; i < cli.nmb_topics; ++i) {
		printf("Topic: %s, Flag: %d\n",
		       cli.topics[i].topic, cli.topics[i].sf_flag);
	}
	printf("\n");
}

void printClients(TClients *cli) {
	if (cli == NULL) {
		fprintf(stderr, "Server is not initialized!\n");
		return;
	}
	if (cli->list_cli == NULL) {
		fprintf(stderr, "Server is empty!\n");
		return;
	}
	for (int i = 0; i < cli->nmb_cli; ++i) {
		printClient(cli->list_cli[i]);
		printSubscriptions(cli->list_cli[i]);
	}
}
// updates the database when the subscriber has crashed.
void forceClose(TClients *cli, int sock) {
	for (int i = 0; i < cli->nmb_cli; ++i) {
		if (cli->list_cli[i].sock == sock) {
			cli->list_cli[i].connect = 0;
			cli->list_cli[i].sock = 0;
			printf("Client (%s) has forcefully closed the connection!\n",
			       cli->list_cli[i].ID);
		}
	}
}

void freeMemory(TClients *cli) {
	for (int i = 0; i < cli->nmb_cli; ++i) {
		if (cli->list_cli != NULL) {
			if (cli->list_cli[i].topics != NULL) {
				free(cli->list_cli[i].topics);
			}
			if (cli->list_cli[i].b_log != NULL) {
				for (int j = 0; j < cli->list_cli[i].nmb_blog; ++j) {
					if (cli->list_cli[i].b_log[j] != NULL) {
						free(cli->list_cli[i].b_log[j]);
					}
				}
				free(cli->list_cli[i].b_log);
			}
		}
	}
	free(cli->list_cli);
}
// parses the UDP message to send to the subscribers
// or to store in the database
TMsg *parseUDP(char *buffer, char *msg, struct sockaddr_in cli_addr) {
	char buf_tmp2[BUFLEN];
	int flag = 0;
	uint8_t sign, decimal;
	uint16_t short_n;
	uint32_t float_n;
	double power;
	memset(buffer, 0, BUFLEN);
	strcpy(buffer, inet_ntoa(cli_addr.sin_addr));
	sprintf(buf_tmp2, "%d", ntohs(cli_addr.sin_port));
	strcat(buffer, ":");
	strcat(buffer, buf_tmp2);
	strcat(buffer, " - ");
	memset(buf_tmp2, 0, BUFLEN);
	TMsg *new = (TMsg *) malloc(sizeof(TMsg));
	memcpy(new->topic, msg, sizeof(char) * 50);
	for (int i = 0; i < 50; ++i) {
		if (new->topic[i] == '\0') {
			flag = 1;
		}
	}
	if (flag != 1) {
		sprintf(buf_tmp2, "%s", new->topic);
		strcat(buffer, buf_tmp2);
	} else {
		strcat(buffer, new->topic);
	}

	strcat(buffer, " - ");
	memcpy(&new->data_type, msg + sizeof(char) * 50, sizeof(uint8_t));
	memcpy(new->message, msg + sizeof(char) * 50 + sizeof(uint8_t), sizeof(char) * 1501);
	if (new->data_type == 0) {
		strcat(buffer, "INT - ");
		uint32_t number;
		memcpy(&sign, new->message, sizeof(uint8_t));
		if (sign == 1) {
			strcat(buffer, "-");
		}
		memcpy(&number, new->message + sizeof(uint8_t), sizeof(uint32_t));
		memset(buf_tmp2, 0, BUFLEN);
		sprintf(buf_tmp2, "%d", ntohl(number));
		strcat(buffer, buf_tmp2);
	} else if (new->data_type == 1) {
		memset(buf_tmp2, 0, BUFLEN);
		strcat(buffer, "SHORT_REAL - ");
		memcpy(&short_n, new->message, sizeof(uint16_t));
		double result = (double) (ntohs(short_n)) / 100;
		sprintf(buf_tmp2, "%.2f", result);
	} else if (new->data_type == 2) {
		strcat(buffer, "FLOAT - ");
		memcpy(&sign, new->message, sizeof(uint8_t));
		memcpy(&float_n, new->message + sizeof(uint8_t), sizeof(uint32_t));
		memcpy(&decimal, new->message + sizeof(uint8_t) + sizeof(uint32_t),
		       sizeof(uint8_t));
		power = pow(10, decimal);
		power = 1 / power;
		if (sign == 1) {
			strcat(buffer, "-");
		}
		memset(buf_tmp2, 0, BUFLEN);
		sprintf(buf_tmp2, "%.15g", ntohl(float_n) * power);
		strcat(buffer, buf_tmp2);
	} else if (new->data_type == 3) {
		strcat(buffer, "STRING - ");
		memset(buf_tmp2, 0, BUFLEN);
		sprintf(buf_tmp2, "%s", new->message);
		strcat(buffer, buf_tmp2);
	}
	return new;
}

int main(int argc, char const *argv[]) {
	int sock_fd, sock_fd_udp, newsock_fd, port_no;
	int i, n, ret, tmp_fdmax, option = 1;

	char buffer[BUFLEN], buf_tmp[BUFLEN], ID[10], *token;

	Toperation *oper;

	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;

	fd_set read_fds;
	fd_set tmp_fds;
	int fd_max;

	if (argc < 2) {
		usage(argv[0]);
	}
	// sets used in
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	// TCP
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sock_fd < 0, "socket");

	port_no = atoi(argv[1]);
	DIE(port_no == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port_no);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(sock_fd, 100);
	DIE(ret < 0, "listen");

	FD_SET(sock_fd, &read_fds);

	sock_fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sock_fd_udp < 0, "socket");

	ret = bind(sock_fd_udp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	FD_SET(sock_fd_udp, &read_fds);
	FD_SET(0, &read_fds);

	if (sock_fd > sock_fd_udp) {
		fd_max = sock_fd;
	} else {
		fd_max = sock_fd_udp;
	}

	TClients *clients = initMem();

	while (1) {
		tmp_fds = read_fds;

		ret = select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fd_max; ++i) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sock_fd) {
					clilen = sizeof(cli_addr);
					newsock_fd = accept(sock_fd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsock_fd < 0, "accept");

					ret = setsockopt(newsock_fd, IPPROTO_TCP, TCP_NODELAY,
					                 (char *) &option, sizeof(option));
					DIE(ret < 0, "TCP_NODELAY");

					FD_SET(newsock_fd, &read_fds);
					tmp_fdmax = fd_max;

					if (newsock_fd > fd_max) {
						fd_max = newsock_fd;
					}

					n = recv(newsock_fd, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					ret = checkSubscription(buffer);
					if (ret < 0) {
						memset(buffer, 0, BUFLEN);
						// invalid id
						strcpy(buffer, "exit 1");
						n = send(newsock_fd, buffer, BUFLEN, 0);
						DIE(n < 0, "send");
						close(newsock_fd);
						FD_CLR(newsock_fd, &read_fds);
						fd_max = tmp_fdmax;
						break;
					}
					token = strtok(buffer, " ");
					strcpy(ID, token);
					if (checkConnected(clients, ID)) {
						memset(buffer, 0, BUFLEN);
						// id is already used
						strcpy(buffer, "exit 2");
						n = send(newsock_fd, buffer, BUFLEN, 0);
						DIE(n < 0, "send");
						close(newsock_fd);
						FD_CLR(newsock_fd, &read_fds);
						fd_max = tmp_fdmax;
						break;
					}
					if (!existingClient(clients, ID, newsock_fd)) {
						addClient(clients, ID, newsock_fd);
					} else {
						ret = reconnect(clients, ID, newsock_fd);
						if (ret == 0) {
							fprintf(stderr, "Error reconnecting!\n");
						}
						printf("Sending backlog...\n");
						sendBacklog(*clients, ID);
					}
					printf("New client (%s) connected from %s:%d\n", ID,
					       inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
					memset(ID, 0, 10);
					printClients(clients);
				} else if (i == sock_fd_udp) {
					n = recvfrom(i, buffer, sizeof(buffer), 0,
					             (struct sockaddr *) &cli_addr, &clilen);
					DIE(n < 0, "recvfrom");

					memset(buf_tmp, 0, BUFLEN);
					TMsg *tmp_msg = parseUDP(buf_tmp, buffer, cli_addr);
					sendUDP(clients, buf_tmp, *tmp_msg);
					memset(buf_tmp, 0, BUFLEN);
				} else if (i == 0) {
					memset(buffer, 0, BUFLEN);
					scanf("%s", buffer);
					if (!strcmp(buffer, "exit")) {
						printf("Deconnecting all clients...\n");
						memset(buf_tmp, 0, BUFLEN);
						// server shutdown!
						strcpy(buf_tmp, "exit 0");
						for (int j = 0; j < clients->nmb_cli; ++j) {
							send(clients->list_cli[j].sock, buf_tmp, BUFLEN, 0);
						}
						printf("OK!\n");
						printf("Exiting...\n");
						close(sock_fd);
						freeMemory(clients);
						free(clients);
						return 0;
					} else {
						printf("Invalid command!\n");
					}
				} else {
					n = recv(i, buffer, sizeof(buffer), 0);

					if (n == 0) {
						close(i);
						FD_CLR(i, &read_fds);
						forceClose(clients, i);
						memset(buffer, 0, BUFLEN);
					} else {
						oper = parseOperation(buffer);
						if (!strcmp(oper->operation, "exit")) {
							deconnect(clients, oper->ID);
							printf("Client %s disconnected.\n", oper->ID);
							close(i);
							FD_CLR(i, &read_fds);
							memset(oper, 0, sizeof(Toperation));
							memset(buffer, 0, BUFLEN);
						} else if (!strcmp(oper->operation, "subscribe")) {
							if (oper->err_topic == 1) {
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "%s", "Invalid topic name (over 50)!");
								n = send(i, buffer, sizeof(buffer), 0);
								DIE(n < 0, "send");
								break;
							}
							if (oper->err_flag == 1) {
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "%s", "Invalid flag (1 or 0)");
								n = send(i, buffer, sizeof(buffer), 0);
								DIE(n < 0, "send");
								break;
							} else {
								printf("Unsubscribing ID (%s) from topic (%s)\n",
								       oper->ID, oper->topic);
								subscribe(clients, oper->ID, oper->topic, oper->sf_flag);
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "%s", "subscribed ");
								strcat(buffer, oper->topic);
								n = send(i, buffer, sizeof(buffer), 0);
								DIE(n < 0, "send");
							}
						} else if (!strcmp(oper->operation, "unsubscribe")) {
							printf("Unsubscribing ID (%s) from topic (%s)\n",
							       oper->ID, oper->topic);
							unsubscribe(clients, oper->ID, oper->topic);
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "%s", "unsubscribed ");
							strcat(buffer, oper->topic);
							n = send(i, buffer, sizeof(buffer), 0);
							DIE(n < 0, "send");
						}
						free(oper);
					}
				}
			}
		}
	}
	return 0;
}