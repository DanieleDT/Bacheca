#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#define user_size 11
#define title_size 257
#define text_size 1025

//used for colored printf
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

struct announce {
	char title[title_size];
	char text[text_size];
	char user[user_size];
	int id;
};

//struct used for the communication
struct msg{
	char cmd[15];
	char user[user_size];
	char text[text_size];
	char title[title_size];
	int id_pass;
};

//struct for a linked list rapresenting a cache of the announcements in the SQLite DB
struct node{
	struct announce* announce;
	struct node* next;
};

char insert_user(char *username, int password);

char login(char * username, int password);

char delete_announcement(int id, char* username);

char insert_announcement(char* user, char* title, char* text);

void init_database();

struct node* list();

struct node* mylist(char* username);

void list_init();

void unlock();

void free_list(struct node* head);

#endif