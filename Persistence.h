#ifndef PERSISTENCE_H
#define PERSISTENCE_H

struct announce {
	char title[1024];
	char text[1024];
	char user[1024];
	int id;
};

struct node{
	struct announce* announce;
	struct node* next;
};

char insert_user(char *username, int password);

char login(char * username, int password);

int hash_code(char* string);

char delete_announcement(int id, char* username);

char insert_announcement(char* user, char* title, char* text);

void init_database();

struct node* list();

struct node* mylist(char* username);

#endif