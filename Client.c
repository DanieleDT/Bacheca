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

#include "Persistence.h"
#include "Utility.h"

#define MAXLINE 1024
#define IP "127.0.0.1"
#define PORT 5195

#define fflush(stdin) while(getchar() != '\n')

int hash_code(char* string){
	//simple hash function for protected passwords
	int hash = 0;
	for(int i = 0; i < strlen(string); i++){
		hash = 31 * hash +  string[i];
	}
	return hash;
}

int main(int argc, char* argv[]){
	struct sockaddr_in server;
	int socket_server;
	socket_server= socket(AF_INET, SOCK_STREAM, 0);
	if(socket < 0){
		printf("Error in socket\n");
		return -1;
	}

	if (setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0){
		printf("setsockopt(SO_REUSEADDR) failed\n");
		exit(0);
	}
	memset(&server, 0, sizeof(server));
	server.sin_addr.s_addr = inet_addr(IP);
	server.sin_family = AF_INET;
	server.sin_port = htons( PORT );

	if (connect(socket_server , (struct sockaddr *)&server , sizeof(server)) == -1){
		printf("%s\n", "Error in connect");
		return -1;
	}
	printf("%s\n", "Benvenuto nella Bacheca");
	printf("\n%s\n", "Scrivi 'help' per conoscere i comandi a disposizione");
	char user[1024];
	char login = 0;
	while(1){
		printf("%s", "\nInserisci un comando: ");
		char command[MAXLINE];
		scanf("%[^\n]", command);
		//printf("command: %s\n", command);
		fflush(stdin);
		if(strcmp("help", command) == 0){
			printf("%s\n", "\nComandi:\n\n- help\n\n- login\n\n- register\n\n- list\n\n- delete\n\n- post\n\n- exit");
		}else if(strcmp("login", command) == 0){
			if(login == 0){
				char username[MAXLINE];
				char password[MAXLINE];
				char msg[3*MAXLINE];

				printf("%s\n", "Inserisci il tuo username:");
				scanf("%[^\n]", username);
				fflush(stdin);
				printf("%s\n", "Inserisci la password:");
				scanf("%[^\n]", password);
				fflush(stdin);
				int hash = hash_code(password);
				//int hash2 = hash_code("a");
				//printf("hash: %d | hash2: %d\n", hash, hash2);

				//printf("%s %s %d\n", command, username, hash);
				//asprintf(command, "%s", command);
				sprintf((char *)&msg, "%s %s %d", command, username, hash);
				//printf("%s\n", msg);
				if(send(socket_server, &msg, sizeof(msg), 0) == -1){
					printf("%s\n", "Error in send");
					exit(0);
				}

				if(recv(socket_server, &login, sizeof(char), 0) == 0){
					printf("%s\n", "Server offline");
					exit(0);
				}
				if(login == 1){
					strcpy(user, username);
					printf("Benvenuto %s!\n", user);
				}else if(login == -1){
					login == 0;
					printf("%s\n", "Hai già effettuato il login");
				}else{
					printf("%s\n", "Username o password errati");
				}
			}else{
				printf("%s\n", "Hai già effettuato il login");
			}
		}else if(strcmp("register", command) == 0){ 
			char username[MAXLINE];
			char password[MAXLINE];
			char msg[3*MAXLINE];
			char password_again[MAXLINE];
			
			printf("%s\n", "Inserisci il tuo username:");
			scanf("%[^\n]", username);
			fflush(stdin);
			printf("%s\n", "Inserisci la password:");
			scanf("%[^\n]", password);
			fflush(stdin);
			printf("%s\n", "Ripeti la password:");
			scanf("%[^\n]", password_again);
			fflush(stdin);
			if(strcmp(password, password_again) != 0) {
				printf("%s\n", "Le password non coincidono");
				continue;
			}
			int hash = hash_code(password);
			sprintf((char *)&msg, "%s %s %d", command, username, hash);
			if(send(socket_server, &msg, sizeof(msg), 0) == -1){
				printf("%s\n", "Error in send");
				exit(0);
			}
			char result;
			if(recv(socket_server, &result, sizeof(char), 0) == 0){
				printf("%s\n", "Server offline");
				exit(0);
			}
			if(result){
				printf("%s\n", "Registrazione effettuata con successo");
			}else{
				printf("%s\n", "Username non disponibile");
			}
		}else if(strcmp("exit", command) == 0){
			char answer[1024];
			printf("%s\n", "Sei sicuro di voler uscire? [S/n]");
			scanf("%[^\n]", answer);
			if(strcmp(answer, "s") == 0 || strcmp(answer, "S") == 0 ){
				printf("%s\n", "Grazie di aver utilizzato la nostra bacheca");
				exit(0);
			}
		}else if(strcmp("list", command) == 0){
			char msg[3*MAXLINE];
			strcpy(msg, "list");
			if(send(socket_server, &msg, sizeof(msg), 0) == -1){
				printf("%s\n", "Error in send");
				exit(0);
			}
			struct announce* announce = malloc(sizeof(struct announce));
			if(announce == NULL){
				printf("%s\n","Error in malloc");
				exit(0);
			}
			long int count = 0;
			while(1){
				if(recv(socket_server, announce, sizeof(struct announce), 0) == 0){
					printf("%s\n", "Error in recv");
					exit(0);
				}
				if(announce-> id == -1){
					break;
				}
				count++;
				printf("\nID: %d\n", announce-> id);
				printf("Titolo: %s\n", announce->title);
				printf("Testo: %s\n", announce->text);
				printf("Utente: %s\n", announce->user);
			}
			free(announce);
			if(count){
				printf("\n%ld annunci disponibili al momento\n", count);
			}else{
				printf("%s\n", "Nessun annuncio disponibile");
			}
		}else if(strcmp("post", command) == 0){
			if(login == 1){
				char title[1024];
				char text[1024];
				printf("%s\n", "Inserisci il titolo dell'annuncio:");
				scanf("%[^\n]", title);
				fflush(stdin);
				printf("%s\n", "Inserisci il testo dell'annuncio:");
				scanf("%[^\n]", text);
				fflush(stdin);
				char msg[3*MAXLINE];
				sprintf((char*)&msg, "post %s %s %s", user, title, text);
				if(send(socket_server, &msg, sizeof(msg), 0) == -1){
					printf("%s\n", "Error in send");
					exit(0);
				}
				char result;
				if(recv(socket_server, &result, sizeof(char), 0) == 0){
					printf("%s\n", "Server offline");
					exit(0);
				}
				if(result == 1){
					printf("%s\n", "Annuncio pubblicato con successo");
				}else if(result == 0){
					printf("%s\n", "Annuncio non pubblicato");
				}else{
					printf("%s\n", "Errore di sincronizzazione con il server, provare a rilanciare il programa");
				}
			}else{
				printf("%s\n", "Devi effettuare il login prima di poter utilizzare questa funzione");
			}
		}else if(strcmp("delete", command) == 0){
			if(login == 1){
				char identifier[1024];
				printf("%s\n", "Inserisci l'ID dell'annuncio:");
				scanf("%[^\n]", identifier);
				fflush(stdin);
				char* rest;
				int id = strtol(identifier, &rest, 10);
				if(strcmp(identifier, rest) == 0){
					printf("ID inserito non valido\n");
				}
				if (id > 0 && id <= INT_MAX){
					printf("%d\n", id);
					char msg[2*MAXLINE];
					sprintf((char*)&msg,"%s %d %s", command, id, user);
					printf("%s\n", msg);
					if(send(socket_server, &msg, sizeof(msg), 0) == -1){
						printf("%s\n", "Error in send");
						exit(0);
					}
					char delete;
					if(recv(socket_server, &delete, sizeof(char), 0) == 0){
						printf("%s\n", "Server offline");
						exit(0);
					}
					if(delete == 0){
						printf("%s\n", "ID non valido");
					}else if(delete == 1){
						printf("%s\n", "Annuncio cancellato");
					}else if(delete == -1){
						printf("%s\n", "L'annuncio non è stato pubblicato da questo account");
					}else {
						printf("%s\n", "Risposta del server errata");
					}

				}else{
					printf("ID inserito non valido\n");
				}
			}else{
				printf("%s\n", "Devi effettuare il login prima di poter utilizzare questa funzione");
			}
		}else if(strcmp("mylist", command) == 0){
			if(login == 1){
				//mylist(user);
				char msg[1024];
				sprintf((char*)&msg, "mylist %s", user);
				if(send(socket_server, &msg, sizeof(msg), 0) == -1){
					printf("%s\n", "Error in send");
					exit(0);
				}
				struct announce* announce = malloc(sizeof(struct announce));
				if(announce == NULL){
					printf("%s\n", "Error in malloc");
					exit(0);
				}
				long int count = 0;
				while(1){
					if(recv(socket_server, announce, sizeof(struct announce), 0) == 0){
						printf("%s\n", "Error in recv");
						exit(0);
					}
					if(announce-> id == -1){
						break;
					}
					count++;
					printf("\nID: %d\n", announce-> id);
					printf("Titolo: %s\n", announce->title);
					printf("Testo: %s\n", announce->text);
					//printf("Utente: %s\n", announce->user);
				}
				free(announce);
				if(count){
					printf("\n%ld annunci pubblicati\n", count);
				}else{
					printf("%s\n", "Nessun annuncio pubblicato");
				}
			}else{
				printf("%s\n", "Devi effettuare il login prima di poter utilizzare questa funzione");
			}
		}else{
			printf("%s\n", "Comando non riconosciuto");
		}
		//memset(command, 0, MAXLINE);
		//printf("dopo send\n");
	}
}