#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface.h"
#include <pthread.h>

struct Node {
    char *name;
    int port;
    struct Node* next;
};

struct Node *head=NULL;

void add_name(char *name, int port) {
	struct Node * new_node = NULL;
	new_node = (struct Node *)malloc(sizeof(struct Node));
	new_node->name = strdup(name);
	// new_node->port = port;
	new_node->next = head;
	head = new_node;
}

bool search_names(char *name)
{
	struct Node * temp = head;
	while (temp!=NULL){
		if(!strcmp(temp->name,name)){
			return true;
		}
		temp = temp->next;
	}
	return false;
// 	Node* current = head;
// 	while (current != NULL)
// 	{
// 		// printf("Current->name: %s\n", current->name);
//      	if (current->name == name){
//           	return true;
//      	}
//      	current = current->next;
// 	}
// 	return false;
}

void display_names(){
    struct Node *temp = head;
    while(temp != NULL){
        printf("DISPLAY: %s\n", temp->name);
        temp = temp->next;
    }
}

int setup_server(const int port)
{
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = port;
	server_address.sin_addr.s_addr = INADDR_ANY;
	
	bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address));
	listen(server_socket, 10);
	return server_socket;
}

void *new_chatroom(void* args){
	int *argptr = (int *) args;
	int port = *argptr;
	int chatroom_socket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in chatroom_address;
	chatroom_address.sin_family = AF_INET;
	chatroom_address.sin_port = port;
	chatroom_address.sin_addr.s_addr = INADDR_ANY;
	
	bind(chatroom_socket, (struct sockaddr *) &chatroom_address, sizeof(chatroom_address));
	listen(chatroom_socket, 10);
	
	// while(1){
	// 	int client_socket = accept(chatroom_socket, NULL, NULL);
	// 	receive_command(client_socket);
	// 	close(client_socket);
	// }
}

void create_command(char *name, int client_socket){
	// int port = (rand() % (99999 - 10000 + 1)) + 10000;
	int port = 123123;
	if (head == NULL){
		// head->name = name;
		add_name(name, port);
		// printf("head->port %s\n",head->port);
	}
	else if (!search_names(name)){ //if the name is not already taken, add it to list and create chatroom with respective port
		add_name(name, port);
		pthread_t thread_id;
		// if (pthread_create(&thread_id, NULL, new_chatroom, (void *)&port) != 0) {
		// 	printf("Uh-oh!\n");
		// 	return -1;
		// }
		// if (pthread_create(&thread_id, NULL, new_chatroom, (void *)&port) != 0){
		// 	printf("Error Mulithreading");
		// 	return;
		// }
		// pthread_create(&thread_id, NULL, new_chatroom, NULL);
		// pthread_join(thread_id, NULL);
	}
	else if (search_names(name)){
		char message[256]="That name is already taken.";
		send(client_socket, message, sizeof(message)+100, 0);
		printf("That name is already taken.\n");
	}
	// display_names();
}

void join_command(char* name, int client_socket){
	//check to see if room name is in char array
	if (!search_names(name)){
		char message[256]="That room name does not exist.";
		send(client_socket, message, sizeof(message)+100, 0);
	}
	//send port number to client so they can connect
}

void delete_func(char* name) {
   struct Node* current = head;
   struct Node* previous = NULL;
    
    if(head == NULL) {
        return;
    }

    while(strcmp(current->name, name)) {
        if(current->next == NULL) {
            return;
        } else {
            previous = current;
            current = current->next;
        }
    }

    if(current == head) {
        head = head->next;
    } else {
        previous->next = current->next;
    }
}

void delete_command(char* name, int client_socket){
	//check to see if name matches up
	// if (search_names(name)){
	// 	struct Node * temp = head;
	// 	while (temp!=NULL){
	// 		if(!strcmp(temp->name,name)){
	// 			temp->next = temp->next->next;
	// 		}
	// 		temp = temp->next;
	// 	}
	// }
	delete_func(name);
	//send warning message
	char warning[256] = "Warning.. deleting chat room";
	send(client_socket, warning, sizeof(warning)+100, 0);
	//close master socket
}

void list_func(int client_socket){
	struct Node *temp = head;
	while(temp != NULL){
	   send(client_socket, temp->name, sizeof(temp->name)+100, 0);
	   temp = temp->next;
    }
}
void receive_command(int client_socket)
{
	char command[256];
	recv(client_socket, &command, sizeof(command), 0);
	// printf("Command Size---->%d\n", sizeof(command));
	// printf("Command Received---->%s\n", command);
	// name = strtok(command, " ");
	// name = strtok(NULL, " ");
	// printf("Name---->%s\n", name);
	touppercase(command, strlen(command) - 1);
	if (strncmp(command, "CREATE", 6) == 0) {
		// printf("Call Create function.\n");
		create_command(command, client_socket);
	}
	else if (strncmp(command, "JOIN", 4) == 0) {
		// printf("Call Join function.\n");
		join_command(command, client_socket);
	}
	else if (strncmp(command, "DELETE", 6) == 0) {
		// printf("Call Delete function.\n");
		delete_command(command, client_socket);
	}
	else if (strncmp(command, "LIST", 4) == 0) {
		// printf("Call List function.\n");
		list_func(client_socket);
	}
	else {
		printf("Unknown Command.\n");
		}
}

//declaring global variables
int chatroom_ports[256];
int chatroom_names[256];

int main(int argc, char** argv) 
{
	// struct Node* head = NULL;
	// head = (struct Node*)malloc(sizeof(struct Node));
	if (argc != 2) {
		fprintf(stderr,
				"usage: enter port number\n");
		exit(1);
	}
	
	int port = atoi(argv[1]);
	int server_socket = setup_server(port);

	while(1){
		int client_socket = accept(server_socket, NULL, NULL);
		receive_command(client_socket);
		close(client_socket);
	}

	return 0;
	
}
