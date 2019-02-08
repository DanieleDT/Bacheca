#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>

#define fflush(stdin) while(getchar() != '\n')

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

#define MAXLINE 1024
#define IP "127.0.0.1"
#define PORT 5195

#define user_size 11
#define title_size 257
#define text_size 1025

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

struct node{
	struct announce* announce;
	struct node* next;
};

int hash_code(char* string){
	//simple hash function for cifrated passwords
	int hash = 0xAAAAAAAA;
	int i = 0;
	for(i = 0; i < MAXLINE; string++, i++){
		hash ^= ((i & 1) == 0) ? (  (hash <<  7) ^ (*string) * (hash >> 3)) :
		(~((hash << 11) + ((*string) ^ (hash >> 5))));
	}
	return hash;
}

char child_job(int i, int request){
	struct sockaddr_in server;
	int socket_server;
	socket_server= socket(AF_INET, SOCK_STREAM, 0);
	if(socket_server < 0){
		printf(RED"Error in socket\n"RESET);
		return -1;
	}
	//connection to the server
	memset(&server, 0, sizeof(server));
	server.sin_addr.s_addr = inet_addr(IP);
	server.sin_family = AF_INET;
	server.sin_port = htons( PORT );
	if (connect(socket_server , (struct sockaddr *)&server , sizeof(server)) == -1){
		printf(RED"%s\n"RESET, "Error in connect");
		return -1;
	}
	//Client ready
	char user[user_size];
	memset(user, 0 , user_size);
	char login = 0;
	struct msg* msg = malloc(sizeof(struct msg));
	memset(msg, 0, sizeof(struct msg));
	if(msg == NULL){
		printf(RED"%s\n"RESET, "Error in msg malloc");
		exit(0);
	}
	int completed = 0;
	while(1){
		if(completed >= request){
			//printf("%s %d\n", "Finito", i);
			exit(0);
		}
		completed++;
		memset(msg, 0, sizeof(struct msg));
		//printf("%s", "\nInserisci un comando: ");
		char command[15];
		memset(command, 0, 15);
		struct timeval tm;
		gettimeofday(&tm, NULL);
		int r = random();
		r = r%7;
		switch(r){
			case 0:
			strcpy(command, "login");
			break;
			case 1:
			strcpy(command, "register");
			break;
			case 2:
			strcpy(command, "exit");
			break;
			case 3:
			strcpy(command, "list");
			break;
			case 4:
			strcpy(command, "post");
			break;
			case 5:
			strcpy(command, "delete");
			break;
			case 6:
			strcpy(command, "mylist");
			break;
		}
		//printf("%s\n", command);
		if(strcmp("help", command) == 0){
			//printf("%s\n", "\nComandi:\n\n- help\n\n- login\n\n- register\n\n- list\n\n- mylist\n\n- delete\n\n- post\n\n- exit");
		}else if(strcmp("login", command) == 0){
			if(login == 0){
				char username[user_size];
				char password[MAXLINE];
				memset(username, 0, user_size);
				memset(password, 0, MAXLINE);
				strcpy(username, "daniele");
				strcpy(password, "daniele");
				int hash = hash_code(password);
				strcpy(msg -> cmd, command);
				strcpy(msg -> user, username);
				msg -> id_pass = hash;
				if(send(socket_server, msg, sizeof(struct msg), 0) != sizeof(struct msg)){
					//printf(RED"%s\n"RESET, "Error in send");
					exit(0);
				}

				if(recv(socket_server, &login, sizeof(char), 0) == 0){
					//printf(RED"%s\n"RESET, "Server offline");
					exit(0);
				}
				if(login == 1){
					strcpy(user, username);
					//printf(GREEN"Benvenuto %s\n"RESET, user);
				}else if(login == -1){
					login == 0;
					//printf(YELLOW"%s\n"RESET, "Hai già effettuato il login");
				}else{
					//printf(RED"%s\n"RESET, "Username o password errati");
				}
			}else{
				//printf(YELLOW"%s\n"RESET, "Hai già effettuato il login");
			}
		}else if(strcmp("register", command) == 0){ 
			char username[user_size+1];
			char password[MAXLINE+1];
			char password_confirm[MAXLINE];
			memset(username, 0, user_size + 1);
			memset(password, 0, MAXLINE + 1);
			memset(password_confirm, 0, MAXLINE);
			strcpy(username, "daniele");
			strcpy(password, "daniele");
			
			int hash = hash_code(password);
			strcpy(msg -> cmd, command);
			strcpy(msg -> user, username);
			msg -> id_pass = hash;
			if(send(socket_server, msg, sizeof(struct msg), 0) != sizeof(struct msg)){
				printf(RED"%s\n"RESET, "Error in send");
				exit(0);
			}
			char result;
			if(recv(socket_server, &result, sizeof(char), 0) == 0){
				printf(RED"%s\n"RESET, "Server offline");
				exit(0);
			}
			if(result){
				//printf(GREEN"%s\n"RESET, "Registrazione effettuata con successo");
			}else{
				//printf(RED"%s\n"RESET, "Username non disponibile");
			}
		}else if(strcmp("exit", command) == 0){
			//exit(0);
		}else if(strcmp("list", command) == 0){
			strcpy(msg -> cmd, command);
			if(send(socket_server, msg, sizeof(struct msg), 0) != sizeof(struct msg)){
				printf(RED"%s\n"RESET, "Error in send");
				exit(0);
			}
			struct announce* announce = malloc(sizeof(struct announce));
			memset(announce, 0, sizeof(struct announce));
			if(announce == NULL){
				printf(RED"%s\n"RESET,"Error in malloc");
				exit(0);
			}
			FILE* list_fd = fopen("./.list.txt", "w+");
			if(list_fd == NULL){
				printf(RED"%s\n"RESET, "Error in fopen");
				exit(0);
			}
			//fprintf(list_fd, YELLOW"%s\n"RESET, "Premi 'q' per uscire.");
			long int count = 0;
			while(1){
				if(recv(socket_server, announce, sizeof(struct announce), MSG_WAITALL) != sizeof(struct announce)){
					printf(RED"%s\n"RESET, "Error in recv list");
					exit(0);
				}
				if(announce-> id == -1){
					break;
				}
				if(announce-> id == 0){
					printf(RED"\n%s\n"RESET, "Error: ID = 0");
					break;
				}
				count++;
				/*
				fprintf(list_fd, "\n/////////////////////////////////////////////\n");
				fprintf(list_fd, "Titolo:\n" );
				fprintf(list_fd, "%s\n\n", announce->title);
				fprintf(list_fd, "Testo:\n" );
				fprintf(list_fd, "%s\n", announce->text);
				fprintf(list_fd, "\nUtente: ");double
				fprintf(list_fd, "%s ", announce->user);
				fprintf(list_fd, "ID: ");
				fprintf(list_fd, "%d\n", announce-> id);
				memset(announce, 0, sizeof(struct announce));
				*/
			}
			free(announce);
			if(fclose(list_fd) != 0){
				printf(RED"%s\n"RESET, "Error in fclose");
				exit(0);
			}
			if(count){
				//printf("\n%ld annunci disponibili al momento\n", count);
			}else{
				//printf("%s\n", "Nessun annuncio disponibile");
			}
		}else if(strcmp("post", command) == 0){
			if(login == 1){
				char title[title_size +1];
				char text[text_size + 1];
				memset(title, 0, title_size + 1);
				memset(text, 0, text_size + 1);
				strcpy(title, "titolo");
				strcpy(text, "testo");
				strcpy(msg -> cmd, command);
				strcpy(msg -> user, user);
				strcpy(msg -> title, title);
				strcpy(msg -> text, text);
				if(send(socket_server, msg, sizeof(struct msg), 0) != sizeof(struct msg)){
					printf(RED"%s\n"RESET, "Error in send");
					exit(0);
				}
				char result;
				if(recv(socket_server, &result, sizeof(char), 0) == 0){
					printf(RED"%s\n"RESET, "Server offline");
					exit(0);
				}
				if(result == 1){
					//printf(GREEN"%s\n"RESET, "Annuncio pubblicato con successo");
				}else if(result == 0){
					//printf(YELLOW"%s\n"RESET, "Annuncio non pubblicato");
				}else{
					//printf(RESET"%s\n"RESET, "Errore di sincronizzazione con il server, provare a rilanciare il programa");
				}
				memset(title, 0, MAXLINE);
				memset(text, 0, MAXLINE);
			}
		}else if(strcmp("delete", command) == 0){
			if(login == 1){
				char identifier[MAXLINE];
				memset(identifier, 0, MAXLINE);
				//printf("%s\n", "Inserisci l'ID dell'annuncio:");
				srand(time(NULL));
				int id = rand();
				if (id > 0 && id <= INT_MAX){
					strcpy(msg -> cmd, command);
					strcpy(msg -> user, user);
					msg -> id_pass = id;
					if(send(socket_server, msg, sizeof(struct msg), 0) != sizeof(struct msg)){
						printf(RED"%s\n"RESET, "Error in send");
						exit(0);
					}
					
					char delete;
					if(recv(socket_server, &delete, sizeof(char), 0) == 0){
						printf(RED"%s\n"RESET, "Server offline");
						exit(0);
					}
				}
			}
		}else if(strcmp("mylist", command) == 0){
			if(login == 1){
				strcpy(msg -> cmd, command);
				strcpy(msg -> user, user);
				if(send(socket_server, msg, sizeof(struct msg), 0) != sizeof(struct msg)){
					printf(RED"%s\n"RESET, "Error in send");
					exit(0);
				}
				struct announce* announce = malloc(sizeof(struct announce));
				memset(announce, 0, sizeof(struct announce));
				if(announce == NULL){
					printf(RED"%s\n"RESET, "Error in malloc");
					exit(0);
				}
				FILE* list_fd = fopen("./.list.txt", "w+");
				if(list_fd == NULL){
					printf(RED"%s\n"RESET, "Error in fopen");
					exit(0);
				}
				fprintf(list_fd, YELLOW"%s\n"RESET, "Premi 'q' per uscire.");
				fprintf(list_fd, "Annunci pubblicati da %s:\n", user);
				long int count = 0;
				while(1){
					if(recv(socket_server, announce, sizeof(struct announce), MSG_WAITALL) != sizeof(struct announce)){
						printf(RED"%s\n"RESET, "Error in recv list");
						exit(0);
					}
					if(announce-> id == -1){
						break;
					}
					if(announce-> id == 0){
						printf(RED"\n%s\n"RESET, "Error: ID = 0");
						break;
					}
					count++;
					fprintf(list_fd, "\n/////////////////////////////////////////////\n");
					fprintf(list_fd, "Titolo:\n" );
					fprintf(list_fd, "%s\n\n", announce->title);
					fprintf(list_fd, "Testo:\n" );
					fprintf(list_fd, "%s\n", announce->text);
					fprintf(list_fd, "\nID: ");
					fprintf(list_fd, "%d\n", announce-> id);
					memset(announce, 0, sizeof(struct announce));
				}
				free(announce);
				if(fclose(list_fd) != 0){
					printf(RED"%s\n"RESET, "Error in fclose");
					exit(0);
				}
				if(count){
					//printf("\n%ld annunci pubblicati\n", count);
				}else{
					//printf("%s\n", "Nessun annuncio pubblicato");
				}
			}
		}else{
			printf(RED"%s\n"RESET, "Comando non riconosciuto");
		}
	}
}

void main(){
	char clients[512];
	char requests[512];
	printf("%s", "Digita il numero di client da avviare: ");
	scanf("%[^\n]", clients);
	fflush(stdin);
	char* rest;
	int value = strtol(clients, &rest, 10);
	if(strcmp("", rest) != 0){
		printf(YELLOW"ID inserito non valido\n"RESET);
		exit(0);
	}
	if (value <= 0){
		printf("Input inserito non valido\n");
		exit(0);
	}
	printf("%s", "Digita il numero di operazioni da svolgere da ogni client: ");
	scanf("%[^\n]", requests);
	fflush(stdin);
	int request = strtol(requests, &rest, 10);
	if(strcmp("", rest) != 0){
		printf(YELLOW"ID inserito non valido\n"RESET);
		exit(0);
	}
	if (value <= 0){
		printf("Input inserito non valido\n");
		exit(0);
	}
	printf("\n");

	pid_t pid;
	for(int i = 0; i < value; i++){
		pid = fork();
		if(pid == -1){
			printf("%s\n", "Error in fork");
			exit(0);
		}
		if(pid == 0){
			child_job(i, request);
			exit(0);
		}
	}
	int status;
	int progress = 0;
	while ((pid = wait(&status)) > 0){
		progress++;
		printf("%d/%d processi hanno terminato\n", progress, value);
	};
	printf("%s\n");
	exit(0);
}