#define _GNU_SOURCE
#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "Persistence.h"

#define size 10
#define database "bacheca.db"

pthread_mutex_t mux_connection = PTHREAD_MUTEX_INITIALIZER;
sqlite3* connections[size];
int start = 0;
int dimension = 0;
//DB connection handled through a ring buffer of already initialized connections, reducing single operations cost

sqlite3* get_connection(){
	sqlite3* connection;
	pthread_mutex_lock(&mux_connection);
	if(dimension == 0){
		//empty buffer
		//create new connection
		if(sqlite3_open(database, &connection)){
			printf("%s\n", "Error in sqlite3_open");
			exit(0);
		}
		//printf("%s\n", "creo connessione");
	}else{
		connection = connections[start];
		start++;
		start = start%size;
		dimension--;
		//printf("%s\n", "prendo connessione dal buffer");
	}
	pthread_mutex_unlock(&mux_connection);
	return connection;
}

void close_connection(sqlite3* connection){
	pthread_mutex_lock(&mux_connection);
	if(dimension >= size){
		//full buffer
		//close connection
		sqlite3_close(connection);
		//printf("%s\n", "chiudo connessione");
	}else{
		//store the connection in the buffer
		connections[(start+dimension)%size] = connection;
		dimension++;
		//printf("%s\n", "metto connessione nel buffer");
	}
	pthread_mutex_unlock(&mux_connection);
}

void init_database(){
	//creates database if it doesn't exist
	printf("TS %d\n",sqlite3_threadsafe());
	sqlite3* connection = get_connection();
	char* sql_utenti = "CREATE TABLE IF NOT EXISTS users(username TEXT PRIMARY KEY NOT NULL, password INTEGER NOT NULL)";

	char* sql_annunci = "CREATE TABLE IF NOT EXISTS announcements(user TEXT NOT NULL,title TEXT,text TEXT,id INTEGER PRIMARY KEY NOT NULL, FOREIGN KEY (user) REFERENCES users(username))";

	if(sqlite3_exec(connection, sql_utenti, 0, 0, NULL)){
		printf("%s\n", "Error in sqlite3_execute");
		exit(0);
	}

	if(sqlite3_exec(connection, sql_annunci, 0, 0, NULL)){
		printf("%s\n", "Error in sqlite3_execute");
		exit(0);
	}
	printf("%s\n", "Database ready");
	close_connection(connection);
}

char insert_user(char* username, int password){
	//return values: 1 user registrated; 0 username already in use
	sqlite3* connection = get_connection();

	char* sql_insert_user = NULL;
	asprintf(&sql_insert_user, "insert into users values('%s', %d);", username, password);
	char* error;
	int result = sqlite3_exec(connection, sql_insert_user, 0, 0, &error);
	if(result != SQLITE_OK){
		if(result = SQLITE_CONSTRAINT){
			//user already in use
			printf("Error: %s\n", error);
			printf("%s, username: %s\n", "Utente già esistente", username);
			close_connection(connection);
			return 0;
		}
		printf("%s\n", "Error in sqlite3_execute");
		exit(0);
	}
	close_connection(connection);
	return 1;
}

char login(char* username, int password){
	//return values: 0 login failed; 1 login successfull
	sqlite3* connection = get_connection();
	char* sql_login = NULL;
	sqlite3_stmt *stmt;
	asprintf(&sql_login, "select count(*) from users where username = '%s' and password = %d;", username, password);
	//printf("%s\n", sql_login);
	if(sqlite3_prepare_v2(connection, sql_login, -1, &stmt, NULL)!= SQLITE_OK){
		printf("%s\n", "error in sqlite3_prepare_v2");
		exit(0);
	}
	if(sqlite3_step(stmt)!= SQLITE_ROW){
		printf("%s\n", "Error in sqlite3_step");
		exit(0);
	}
	int rowcount = sqlite3_column_int(stmt, 0);
	char result;
	if(rowcount == 0){
		result = 0;
	}else if(rowcount == 1){
		result = 1;
	}else{
		printf("%s\n", "Error in login result set");
		exit(0);
	}
	close_connection(connection);
	return result;
}

/*
int hash_code(char* string){
	//simple hash function for protected passwords
	int hash = 0;
	for(int i = 0; i < strlen(string); i++){
		hash = 31 * hash +  string[i];
	}
	return hash;
}
*/

char insert_announcement(char* user, char* title, char* text){
	//return values: 1 insert successfull, 0 user non existing
	sqlite3* connection = get_connection();
	sqlite3_stmt *stmt;

	char* sql_insert_announcements = NULL;
	char* sql_check_user = NULL;
	asprintf(&sql_check_user, "select count(*) from users where username = '%s'", user);

	if(sqlite3_prepare_v2(connection, sql_check_user, -1, &stmt, NULL)!= SQLITE_OK){
		printf("%s\n", "Error in sqlite3_prepare_v2");
		exit(0);
	}
	if(sqlite3_step(stmt)!= SQLITE_ROW){
		printf("%s\n", "Error in sqlite3_step");
		exit(0);
	}
	int rowcount = sqlite3_column_int(stmt, 0);
	if(rowcount == 0){
		//printf("%s\n", "User non existing");
		close_connection(connection);
		return 0;
	}else if(rowcount == 1){
		//user exists
		asprintf(&sql_insert_announcements, "INSERT INTO announcements(user, text, title) VALUES('%s', '%s', '%s');", user, text, title);
		int result = sqlite3_exec(connection, sql_insert_announcements, 0, 0, NULL);
		if(result != SQLITE_OK){
			printf("%s\n", "Error in sqlite3_execute");
			exit(0);
		}
		close_connection(connection);
		//printf("%s\n", "Announcement added correctly");
		return 1;
	}else{
		printf("%s\n", "Error in insert result set");
		exit(0);
	}
}

char delete_announcement(int id, char* username){
	//return values: 1 ann deleted, 0 ID not valid, -1 user not the owner
	sqlite3* connection = get_connection();
	char* sql_check_announcement = NULL;
	sqlite3_stmt *stmt;
	asprintf(&sql_check_announcement, "select user from announcements where id = %d", id);

	if(sqlite3_prepare_v2(connection, sql_check_announcement, -1, &stmt, NULL)!= SQLITE_OK){
		printf("%s\n", "Error in sqlite3_prepare_v2");
		exit(0);
	}
	if(sqlite3_step(stmt)!= SQLITE_ROW){
		//id not valid
		printf("%s\n", "ID not valid");
		close_connection(connection);
		return 0;
	}else{
		//id valid
		const char* user = sqlite3_column_text(stmt, 0);
		printf("Owner: %s\n", user);
		if(strcmp(user, username) == 0){
			char* sql_delete_announcement = NULL;		
			//asprintf(&sql_delete_announcement, "delete from announcements where user = %s;", id, username);
			asprintf(&sql_delete_announcement, "delete from announcements where id = %d and user = '%s';", id, username);
			int result = sqlite3_exec(connection, sql_delete_announcement, 0, 0, NULL);
			if(result != SQLITE_OK){
				printf("%s\n", "Error in sqlite3_exec");
				exit(0);
			}
			printf("%s\n", "Announcement deleted");
			close_connection(connection);
			return 1;
		}else{
			printf("%s\n", "User not owner");
			close_connection(connection);
			return -1;
		}
	}
}

struct node* list(){
	sqlite3* connection = get_connection();
	char* sql_list = NULL;
	sqlite3_stmt *stmt;
	asprintf(&sql_list, "select title, text, user, id from announcements");

	if(sqlite3_prepare_v2(connection, sql_list, -1, &stmt, NULL)!= SQLITE_OK){
		printf("%s\n", "Error in sqlite3_prepare_v2");
		exit(0);
	}

	struct node* head = NULL;
	
	while(sqlite3_step(stmt) == SQLITE_ROW){
		struct node* new_node = malloc(sizeof(struct node));
		struct announce* announce = malloc(sizeof(struct announce));
		if(new_node == NULL || announce == NULL){
			printf("%s\n", "Error in malloc");
		}
		//ID
		announce->id = sqlite3_column_int(stmt, 3);
		//Title
		strcpy(announce->title, sqlite3_column_text(stmt, 0));
		//Text
		strcpy(announce->text, sqlite3_column_text(stmt, 1));
		//User
		strcpy(announce->user, sqlite3_column_text(stmt, 2));
		
		new_node -> announce = announce;
		new_node -> next = head;
		head = new_node;

		/*printf("ID: %d\n", head -> announce-> id);
		printf("Titolo: %s\n", head -> announce->title);
		printf("Testo: %s\n", head-> announce->text);
		printf("Utente: %s\n", head-> announce->user);
		*/
	}
	return head;
}

struct node* mylist(char* username){
	sqlite3* connection = get_connection();
	char* sql_list = NULL;
	sqlite3_stmt *stmt;
	asprintf(&sql_list, "select title, text, id from announcements where user = '%s'", username);

	if(sqlite3_prepare_v2(connection, sql_list, -1, &stmt, NULL)!= SQLITE_OK){
		printf("%s\n", "Error in sqlite3_prepare_v2");
		exit(0);
	}

	struct node* head = NULL;
	
	while(sqlite3_step(stmt) == SQLITE_ROW){
		struct node* new_node = malloc(sizeof(struct node));
		struct announce* announce = malloc(sizeof(struct announce));
		if(new_node == NULL || announce == NULL){
			printf("%s\n", "Error in malloc");
		}
		//ID
		announce->id = sqlite3_column_int(stmt, 2);
		//Title
		strcpy(announce->title, sqlite3_column_text(stmt, 0));
		//Text
		strcpy(announce->text, sqlite3_column_text(stmt, 1));
		//User
		strcpy(announce->user, username);
		
		new_node -> announce = announce;
		new_node -> next = head;
		head = new_node;

		/*
		printf("ID: %d\n", head -> announce-> id);
		printf("Titolo: %s\n", head -> announce->title);
		printf("Testo: %s\n", head-> announce->text);
		printf("Utente: %s\n", head-> announce->user);
		*/
		
	}
	return head;
}