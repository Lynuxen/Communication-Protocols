#include <stdlib.h>
#include <stdio.h>
// used for parsing an UDP message
typedef struct message {
	char topic[50];
	uint8_t data_type;
	char message[1501];
} TMsg;
// used for checking if subscribed
typedef struct topic {
	char topic[50];
	int sf_flag;
} TTopic;
// a single subscriber
// topics is filled with subscriptions
// b_log is the messages a subscriber recieved while offline
typedef struct client {
	char ID[10];
	int sock;
	int connect;
	int nmb_topics;
	int b_capacity;
	int t_capacity;
	int nmb_blog;
	char **b_log;
	TTopic *topics;
} TClient;
// main database
typedef struct clients {
	int nmb_cli;
	int capacity;
	TClient *list_cli;
} TClients;
// used for parsing an operation from the subscriber
typedef struct operation {
	char operation[15];
	char topic[50];
	char ID[10];
	int sf_flag;
	int err_topic;
	int err_flag;
} Toperation;