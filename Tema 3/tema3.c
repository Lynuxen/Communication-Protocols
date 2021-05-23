#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

typedef struct user_info {
	char username[INFOLEN];
	char password[INFOLEN];
} user_info;

typedef struct book_info {
	char title[BOOKLEN];
	char author[BOOKLEN];
	char genre[BOOKLEN];
	char publisher[BOOKLEN];
	char page_count[BOOKLEN];
} book_info;

// read from stdin and store it in the struct user_info
user_info *read_user_info() {
	user_info *tmp = malloc(sizeof(user_info));
	printf("username=");
	fgets(tmp->username, INFOLEN, stdin);
	tmp->username[strcspn(tmp->username, "\n")] = 0;
	printf("password=");
	fgets(tmp->password, INFOLEN, stdin);
	tmp->password[strcspn(tmp->password, "\n")] = 0;
	return tmp;
}

// serialize the user_info using parson
char *serialize_user_info(user_info info) {
	char *s_string;
	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);
	json_object_set_string(root_object, "username", info.username);
	json_object_set_string(root_object, "password", info.password);
	s_string = json_serialize_to_string_pretty(root_value);
	json_value_free(root_value);
	return s_string;
}

// read from stdin and store it in the struct read_book_info
book_info *read_book_info() {
	book_info *tmp = malloc(sizeof(book_info));
	printf("title=");
	fgets(tmp->title, BOOKLEN, stdin);
	printf("author=");
	fgets(tmp->author, BOOKLEN, stdin);
	printf("genre=");
	fgets(tmp->genre, BOOKLEN, stdin);
	printf("publisher=");
	fgets(tmp->publisher, BOOKLEN, stdin);
	printf("page_count=");
	fgets(tmp->page_count, BOOKLEN, stdin);
	tmp->title[strcspn(tmp->title, "\n")] = 0;
	tmp->author[strcspn(tmp->author, "\n")] = 0;
	tmp->genre[strcspn(tmp->genre, "\n")] = 0;
	tmp->publisher[strcspn(tmp->publisher, "\n")] = 0;
	tmp->page_count[strcspn(tmp->page_count, "\n")] = 0;
	return tmp;
}

// serialize the book info using the parson
char *serialize_book_info(book_info info) {
	char *s_string;
	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);
	json_object_set_string(root_object, "title", info.title);
	json_object_set_string(root_object, "author", info.author);
	json_object_set_string(root_object, "genre", info.genre);
	json_object_set_string(root_object, "page_count", info.page_count);
	json_object_set_string(root_object, "publisher", info.publisher);
	s_string = json_serialize_to_string_pretty(root_value);
	json_value_free(root_value);
	return s_string;
}

// prints a json error
void print_json_error(char *err) {
	JSON_Value *root_value = json_parse_string(err);
	JSON_Object *root_object = json_value_get_object(root_value);
	printf("Error: %s\n\n", json_object_get_string(root_object, "error"));
}

// used for get_book
void print_book(char *bk) {
	JSON_Value *root_value = json_parse_string(bk);
	JSON_Object *root_object = json_value_get_object(root_value);
	printf("%-20s: %-10s\n", "Title", json_object_get_string(root_object, "title"));
	printf("%-20s: %-10s\n", "Author", json_object_get_string(root_object, "author"));
	printf("%-20s: %-10s\n", "Genre", json_object_get_string(root_object, "genre"));
	printf("%-20s: %-10.0f\n", "Number of pages", json_object_get_number(root_object, "page_count"));
	printf("%-20s: %-10s\n\n", "Publisher", json_object_get_string(root_object, "publisher"));
}

// used for get_books()
void print_books(char *bks) {
	JSON_Value *root_value;
	JSON_Array *books;
	JSON_Object *book;

	root_value = json_parse_string(bks);
	if (json_value_get_type(root_value) != JSONArray) {
		printf("Error parsing books! Exiting now...\n");
		exit(-2);
	}
	books = json_value_get_array(root_value);
	printf("%s\t%s\n", "ID", "Title");
	for (int i = 0; i < json_array_get_count(books); ++i) {
		book = json_array_get_object(books, i);
		printf("%.0f\t%.10s\n",
		       json_object_get_number(book, "id"),
		       json_object_get_string(book, "title"));
	}
	printf("\n");
}

int main(void) {
	int sock_fd;
	char buffer[BUFLEN];
	char *URL, *HOST, *PAYLOAD_TYPE, *cookie = NULL, *response, *message;
	char *serialized_string, *extracted, *JWT_token = NULL;


	HOST = "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com";
	PAYLOAD_TYPE = "application/json";
	URL = malloc(INFOLEN * sizeof(char));
	user_info *new;
	book_info *book;

	while (1) {
		fgets(buffer, BUFLEN, stdin);
		buffer[strcspn(buffer, "\n")] = 0;
		sock_fd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);
		if (sock_fd < 0) {
			fprintf(stderr, "Unable to connect!");
			return -1;
		}
		if (!strcmp(buffer, "register")) {
			new = read_user_info();
			serialized_string = serialize_user_info(*new);
			strcpy(URL, "/api/v1/tema/auth/register");
			message = compute_post_request(HOST, URL, PAYLOAD_TYPE,
			                               serialized_string, NULL, NULL);
			send_to_server(sock_fd, message);
			response = receive_from_server(sock_fd);
			extracted = basic_extract_json_response(response);
			if (extracted == NULL) {
				printf("Registered!\n");
			} else {
				print_json_error(extracted);
			}
		} else if (!strcmp(buffer, "login")) {
			new = read_user_info();
			serialized_string = serialize_user_info(*new);
			strcpy(URL, "/api/v1/tema/auth/login");
			message = compute_post_request(HOST, URL, PAYLOAD_TYPE,
			                               serialized_string, NULL, NULL);
			send_to_server(sock_fd, message);
			response = receive_from_server(sock_fd);
			extracted = basic_extract_json_response(response);
			if (extracted == NULL) {
				printf("Logged in!\n");
				char *tmp = strstr(response, "connect");
				cookie = malloc(INFOLEN * sizeof(char));
				strcpy(cookie, tmp);
				cookie = strtok(cookie, ";");
			} else {
				print_json_error(extracted);
			}
		} else if (!strcmp(buffer, "logout")) {
			strcpy(URL, "/api/v1/tema/auth/logout");
			message = compute_get_request(HOST, URL, NULL, cookie, NULL);
			send_to_server(sock_fd, message);
			response = receive_from_server(sock_fd);
			extracted = basic_extract_json_response(response);
			if (extracted == NULL) {
				printf("Log out successful!\n");
				memset(cookie, 0, INFOLEN);
				JWT_token = NULL;
			} else {
				print_json_error(extracted);
			}
		} else if (!strcmp(buffer, "enter_library")) {
			strcpy(URL, "/api/v1/tema/library/access");
			message = compute_get_request(HOST, URL, NULL, cookie, NULL);
			send_to_server(sock_fd, message);
			response = receive_from_server(sock_fd);
			if (strstr(response, "error")) {
				extracted = basic_extract_json_response(response);
				print_json_error(extracted);
				continue;
			} else {
				printf("Entered!\n");
				JWT_token = basic_extract_json_response(response);
				JWT_token = strtok(JWT_token, "{:\"");
				JWT_token = strtok(NULL, "{:\"");
			}
		} else if (!strcmp(buffer, "get_books")) {
			strcpy(URL, "/api/v1/tema/library/books");
			message = compute_get_request(HOST, URL, NULL, cookie, JWT_token);
			send_to_server(sock_fd, message);
			response = receive_from_server(sock_fd);
			extracted = basic_extract_json_list_response(response);
			if (strstr(response, "error") != NULL) {
				extracted = basic_extract_json_response(response);
				print_json_error(extracted);
				continue;
			} else if (extracted != NULL) {
				print_books(extracted);
			}
		} else if (!strcmp(buffer, "get_book")) {
			strcpy(URL, "/api/v1/tema/library/books/");
			memset(buffer, 0, BUFLEN);
			printf("id=");
			fgets(buffer, BUFLEN, stdin);
			buffer[strcspn(buffer, "\n")] = 0;
			strcat(URL, buffer);
			message = compute_get_request(HOST, URL, NULL, cookie, JWT_token);
			send_to_server(sock_fd, message);
			response = receive_from_server(sock_fd);
			extracted = basic_extract_json_response(response);
			if (strstr(response, "error") != NULL) {
				extracted = basic_extract_json_response(response);
				print_json_error(extracted);
				continue;
			} else if (extracted != NULL) {
				print_book(extracted);
			}
		} else if (!strcmp(buffer, "add_book")) {
			strcpy(URL, "/api/v1/tema/library/books");
			book = read_book_info();
			serialized_string = serialize_book_info(*book);
			message = compute_post_request(HOST, URL, PAYLOAD_TYPE,
			                               serialized_string, cookie, JWT_token);
			send_to_server(sock_fd, message);
			response = receive_from_server(sock_fd);
			extracted = basic_extract_json_response(response);
			if (strstr(response, "Bad Request") != NULL) {
				printf("Bad request!\n");
			} else if (strstr(response, "Too Many Requests") != NULL) {
				printf("Too many requests, please try again later.\n");
			} else if (extracted == NULL) {
				printf("Book successfully added!\n");
			} else {
				print_json_error(extracted);
			}
		} else if (!strcmp(buffer, "delete_book")) {
			strcpy(URL, "/api/v1/tema/library/books/");
			memset(buffer, 0, BUFLEN);
			printf("id=");
			fgets(buffer, BUFLEN, stdin);
			buffer[strcspn(buffer, "\n")] = 0;
			strcat(URL, buffer);
			message = compute_delete_request(HOST, URL, cookie, JWT_token);
			send_to_server(sock_fd, message);
			response = receive_from_server(sock_fd);
			extracted = basic_extract_json_response(response);
			if (strstr(response, "Bad Request") != NULL) {
				printf("Error: You are not logged in!\n");
			} else if (extracted == NULL) {
				printf("Book successfully deleted!\n");
			} else {
				print_json_error(extracted);
			}
		} else if (!strcmp(buffer, "exit")) {
			close_connection(sock_fd);
			return 0;
		} else {
			fprintf(stdout, "INVALID COMMAND!\n");
		}
	}
	return 0;
}