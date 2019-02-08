#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "Persistence.h"

#ifndef SQLITE_FILE
#include <sqlite3.h>
#else
#include "./sqlite/sqlite3.h"
#endif

#define database "bacheca.db"

pthread_rwlock_t list_lock = PTHREAD_RWLOCK_INITIALIZER;

sqlite3* conn = NULL;

char list_erased = 0;
struct node* list_cache;

sqlite3* get_connection(){
//returns a connection to the database
	if(conn == NULL){
		if(sqlite3_open(database, &conn)){
			printf(RED"%s\n"RESET, "Error in sqlite3_open");
			//pthread_mutex_unlock(&mux_connection);
			pthread_exit(0);
		}
	}
	return conn;
}

//rw lock functions to access the cached announcements list
void read_lock(){
	while(1){
		int result = pthread_rwlock_rdlock(&list_lock);
		if(result == 0) break;
		if(result == EAGAIN) continue;
		printf(RED"%s\n"RESET, "Error in rdlock");
		pthread_exit(0);
	}
}

void write_lock(){
	if(pthread_rwlock_wrlock(&list_lock) != 0){
		printf(RED"%s\n"RESET, "Error in wrlock");
		pthread_exit(0);
	}
}

void unlock(){
	if(pthread_rwlock_unlock(&list_lock) != 0){
		printf(RED"%s\n"RESET, "Error in unlock");
		pthread_exit(0);
	}
}

////////////////////////////////////////////////////////////////////////////

char insert_user(char* username, int password){
	//return values: 1 user registrated; 0 username already in use
	sqlite3* connection = get_connection();
	char* sql_insert_user = NULL;
	asprintf(&sql_insert_user, "insert into users values('%s', %d);", username, password);
	int result = sqlite3_exec(connection, sql_insert_user, 0, 0, NULL);
	free(sql_insert_user);
	if(result != SQLITE_OK){
		if(result = SQLITE_CONSTRAINT){
			//user already in use
			return 0;
		}
		printf(RED"%s\n"RESET, "Error in sqlite3_execute");
		pthread_exit(0);
	}
	return 1;
}

char login(char* username, int password){
	//return values: 0 login failed; 1 login successfull
	sqlite3* connection = get_connection();
	char* sql_login = NULL;
	sqlite3_stmt *stmt;
	asprintf(&sql_login, "select count(*) from users where username = '%s' and password = %d;", username, password);
	if(sqlite3_prepare_v2(connection, sql_login, -1, &stmt, NULL)!= SQLITE_OK){
		printf(RED"%s\n"RESET, "Error in sqlite3_prepare_v2");
		free(sql_login);
		pthread_exit(0);
	}
	free(sql_login);
	if(sqlite3_step(stmt)!= SQLITE_ROW){
		printf(RED"%s\n"RESET, "Error in sqlite3_step");
		sqlite3_finalize(stmt);
		pthread_exit(0);
	}
	int rowcount = sqlite3_column_int(stmt, 0);
	char result;
	if(rowcount == 0){
		result = 0;
	}else if(rowcount == 1){
		result = 1;
	}else{
		printf(RED"%s\n"RESET, "Error in login result set");
		sqlite3_finalize(stmt);
		pthread_exit(0);
	}
	sqlite3_finalize(stmt);
	return result;
}

char insert_announcement(char* user, char* title, char* text){
	//return values: 1 insert successfull, 0 user non existing
	sqlite3* connection = get_connection();
	sqlite3_stmt *stmt;

	char* sql_insert_announcements = NULL;
	char* sql_check_user = NULL;
	asprintf(&sql_check_user, "select count(*) from users where username = '%s'", user);
	if(sqlite3_prepare_v2(connection, sql_check_user, -1, &stmt, NULL)!= SQLITE_OK){
		printf(RED"%s\n"RESET, "Error in sqlite3_prepare_v2");
		free(sql_check_user);
		pthread_exit(0);
	}
	free(sql_check_user);
	if(sqlite3_step(stmt)!= SQLITE_ROW){
		printf(RED"%s\n"RESET, "Error in sqlite3_step");
		sqlite3_finalize(stmt);
		pthread_exit(0);
	}
	int rowcount = sqlite3_column_int(stmt, 0);
	if(rowcount == 0){
		sqlite3_finalize(stmt);
	}else if(rowcount == 1){
		//user exists
		asprintf(&sql_insert_announcements, "INSERT INTO announcements(user, text, title) VALUES('%s', '%s', '%s');", user, text, title);
		int result = sqlite3_exec(connection, sql_insert_announcements, 0, 0, NULL);
		free(sql_insert_announcements);
		if(result != SQLITE_OK){
			printf(RED"%s\n"RESET, "Error in sqlite3_execute");
			sqlite3_finalize(stmt);
			pthread_exit(0);
		}
		sqlite3_finalize(stmt);
	}else{
		printf(RED"%s\n"RESET, "Error in insert result set");
		sqlite3_finalize(stmt);
		pthread_exit(0);
	}
	struct node* head;
	write_lock();
	//list not valid anymore
	head = list_cache;
	list_cache = NULL;
	list_erased = 1;
	unlock();
	//Free of the old list
	free_list(head);
	return rowcount;
}

char delete_announcement(int id, char* username){
	//return values: 1 ann deleted, 0 ID not valid, -1 user not the owner
	sqlite3* connection = get_connection();
	char* sql_check_announcement = NULL;
	sqlite3_stmt *stmt;
	asprintf(&sql_check_announcement, "select user from announcements where id = %d", id);
	if(sqlite3_prepare_v2(connection, sql_check_announcement, -1, &stmt, NULL)!= SQLITE_OK){
		printf("%s\n", "Error in sqlite3_prepare_v2 insert_announcement");
		free(sql_check_announcement);
		pthread_exit(0);
	}
	free(sql_check_announcement);
	if(sqlite3_step(stmt)!= SQLITE_ROW){
		//id not valid
		sqlite3_finalize(stmt);
		return 0;
	}else{
		//id valid
		const char* user = sqlite3_column_text(stmt, 0);
		if(strcmp(user, username) == 0){
			char* sql_delete_announcement = NULL;		
			asprintf(&sql_delete_announcement, "delete from announcements where id = %d and user = '%s';", id, username);
			int result = sqlite3_exec(connection, sql_delete_announcement, 0, 0, NULL);
			free(sql_delete_announcement);
			if(result != SQLITE_OK){
				printf("%s\n", "Error in sqlite3_exec insert_announcement");
				sqlite3_finalize(stmt);
				pthread_exit(0);
			}
			sqlite3_finalize(stmt);
			struct node* head;
			struct node* prev;
			write_lock();
			head = list_cache;
			if (head != NULL && (head->announce->id) == id){
				list_cache = head->next;
				unlock();
				free(head -> announce);
				free(head);
				return 1;
			} 
			while(1){
				if(head == NULL || (head -> announce -> id) == id){
					break;
				}
				prev = head;
				head = head -> next;
			}
			if(head != NULL){
				prev -> next = head -> next;
			}
			unlock();
			free(head -> announce);
			free(head);
			return 1;
		}else{
			sqlite3_finalize(stmt);
			return -1;
		}
	}
}

struct node* list(){
	//return pointer to the list's head
	read_lock();
	while(1){
		if(list_erased == 1){
			unlock();
			write_lock();
			if(list_erased == 1){
				//list need to be rebuilt
				list_init();
			}
			unlock();
			read_lock();
		}
		if(list_erased != 1){
			return list_cache;
		}
	}
}

void free_list(struct node* head){
	//free of a list after a post announcement
	while(head != NULL){
		free(head -> announce);
		struct node* old_head = head;
		head = head -> next;
		free(old_head);
	}
}

void list_init(){
	list_cache = NULL;
	sqlite3* connection = get_connection();
	char* sql_list = NULL;
	sqlite3_stmt *stmt;
	asprintf(&sql_list, "select title, text, user, id from announcements");
	if(sqlite3_prepare_v2(connection, sql_list, -1, &stmt, NULL)!= SQLITE_OK){
		printf("%s\n", "Error in sqlite3_prepare_v2 list_init");
		free(sql_list);
		pthread_exit(0);
	}
	free(sql_list);
	while(sqlite3_step(stmt) == SQLITE_ROW){
		struct node* new_node = malloc(sizeof(struct node));
		struct announce* announce = malloc(sizeof(struct announce));
		memset(new_node, 0, sizeof(struct node));
		memset(announce, 0, sizeof(struct announce));
		if(new_node == NULL || announce == NULL){
			printf("%s\n", "Error in malloc");
			sqlite3_finalize(stmt);
			pthread_exit(0);
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
		new_node -> next = list_cache;
		list_cache = new_node;
	}
	sqlite3_finalize(stmt);
	list_erased = 0;
}

void init_database(){
	//creates database if it doesn't exist
	sqlite3* connection = get_connection();
	char* sql_utenti = "CREATE TABLE IF NOT EXISTS users(username TEXT PRIMARY KEY NOT NULL, password INTEGER NOT NULL)";

	char* sql_annunci = "CREATE TABLE IF NOT EXISTS announcements(user TEXT NOT NULL,title TEXT,text TEXT,id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, FOREIGN KEY (user) REFERENCES users(username))";

	if(sqlite3_exec(connection, sql_utenti, 0, 0, NULL)){
		printf("%s\n", "Error in sqlite3_execute");
		exit(0);
	}

	if(sqlite3_exec(connection, sql_annunci, 0, 0, NULL)){
		printf("%s\n", "Error in sqlite3_execute");
		exit(0);
	}
	printf(GREEN"%s\n"RESET, "Database ready");
}