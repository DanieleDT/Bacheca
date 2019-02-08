#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <limits.h>

#include "Persistence.h"

#define MAXLINE 1024
#define IP "127.0.0.1"
#define PORT 5195

#define fflush(stdin) while(getchar() != '\n')

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

int main(int argc, char* argv[]){
	//socket request
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
		printf(RED"%s\n"RESET, "Server non disponibile al momento");
		return -1;
	}
	//Client ready
	printf(GREEN"%s\n"RESET, "Benvenuto nella Bacheca");
	printf(BLUE"\n%s\n"RESET, "Scrivi 'help' per conoscere i comandi a disposizione");
	char user[user_size];
	memset(user, 0 , user_size);
	char login = 0;
	struct msg* msg = malloc(sizeof(struct msg));
	memset(msg, 0, sizeof(struct msg));
	if(msg == NULL){
		printf(RED"%s\n"RESET, "Error in msg malloc");
		exit(0);
	}
	while(1){
		memset(msg, 0, sizeof(struct msg));
		if(login){
			printf("\n%s, inserisci un comando: ", user);
		}else{
			printf("%s", "\nInserisci un comando: ");
		}
		char command[15];
		memset(command, 0, 15);
		scanf("%[^\n]", command);
		fflush(stdin);
		if(strcmp("help", command) == 0){
			if(login){
				printf("%s\n", "\nComandi:\n\n- help\n\n- logout\n\n- list\n\n- mylist\n\n- delete\n\n- post\n\n- exit");
			}else{
				printf("%s\n", "\nComandi:\n\n- help\n\n- login\n\n- register\n\n- list\n\n- exit");
			}
		}else if(strcmp("login", command) == 0){
			if(login == 0){
				char username[user_size];
				char password[MAXLINE];
				memset(username, 0, user_size);
				memset(password, 0, MAXLINE);
				printf("%s", "Inserisci il tuo username: ");
				scanf("%[^\n]", username);
				fflush(stdin);
				int len = strlen(username);
				if(len < 4){
					printf(YELLOW"%s\n"RESET, "L'username deve contenere almeno 4 caratteri");
					continue;
				}
				if(len > user_size - 1){
					printf(YELLOW"%s\n"RESET, "L'username può contenere al più 10 caratteri");
					continue;
				}
				if(strchr(username, ' ') != NULL){
					printf(YELLOW"%s\n"RESET, "L'username non può contenere spazi");
					continue;
				}
				printf("%s\n", "Inserisci la password:");
				system("stty -echo");
				scanf("%[^\n]", password);
				fflush(stdin);
				system("stty echo");
				printf("\n");
				if(strlen(password) < 4){
					printf(YELLOW"%s\n"RESET, "La password deve contenere almeno 6 caratteri");
					continue;
				}
				int hash = hash_code(password);
				strcpy(msg -> cmd, command);
				strcpy(msg -> user, username);
				msg -> id_pass = hash;
				if(send(socket_server, msg, sizeof(struct msg), 0) != sizeof(struct msg)){
					printf(RED"%s\n"RESET, "Error in send");
					exit(0);
				}

				if(recv(socket_server, &login, sizeof(char), 0) == 0){
					printf(RED"%s\n"RESET, "Server offline");
					exit(0);
				}
				if(login == 1){
					strcpy(user, username);
					printf(GREEN"Benvenuto, %s\n"RESET, user);
				}else if(login == -1){
					login == 0;
					printf(YELLOW"%s\n"RESET, "Hai già effettuato il login");
				}else{
					printf(RED"%s\n"RESET, "Username o password errati");
				}
			}else{
				printf(YELLOW"%s\n"RESET, "Comando non disponibile: hai già effettuato il login");
			}
		}else if(strcmp("register", command) == 0){
			if(!login){
				char username[user_size+1];
				char password[MAXLINE+1];
				char password_confirm[MAXLINE];
				memset(username, 0, user_size + 1);
				memset(password, 0, MAXLINE + 1);
				memset(password_confirm, 0, MAXLINE);

				printf("%s\n", "Inserisci il tuo username:");
				scanf("%[^\n]", username);
				fflush(stdin);
				int len = strlen(username);
				if(len < 4){
					printf(YELLOW"%s\n"RESET, "L'username deve contenere almeno 4 caratteri");
					continue;
				}
				if(len > user_size -1){
					printf(YELLOW"%s%d%s\n"RESET, "L'username può contenere al più ", user_size -1, " caratteri");
					continue;
				}
				if(strchr(username, ' ') != NULL){
					printf(YELLOW"%s\n"RESET, "L'username non può contenere spazi");
					continue;
				}

				printf("%s\n", "Inserisci la password:");
				system("stty -echo");
				scanf("%[^\n]", password);
				fflush(stdin);
				system("stty echo");

				printf("%s\n", "Ripeti la password:");
				system("stty -echo");
				scanf("%[^\n]", password_confirm);
				fflush(stdin);
				system("stty echo");
				int lenght = strlen(password);
				if(lenght < 1){
					printf(YELLOW"%s\n"RESET, "Password vuota non ammessa");
					continue;
				}
				if(lenght >= MAXLINE){
					printf(YELLOW"%s\n"RESET, "Password troppo lunga");
					continue;
				}
				if(strchr(password, ' ') != NULL){
					printf(YELLOW"%s\n"RESET, "La password non può contenere spazi");
					continue;
				}
				if(strcmp(password, password_confirm) != 0) {
					printf(YELLOW"%s\n"RESET, "Le password non coincidono");
					continue;
				}
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
					printf(GREEN"%s\n"RESET, "Registrazione effettuata con successo");
				}else{
					printf(RED"%s\n"RESET, "Username non disponibile");
				}
			}else{
				printf(YELLOW"%s\n"RESET, "Hai già effettuato il login");
			}	
		}else if(strcmp("exit", command) == 0){
			char answer[MAXLINE];
			memset(answer, 0 , MAXLINE);
			printf("%s", "Sei sicuro di voler uscire? [S/n]: ");
			scanf("%[^\n]", answer);
			fflush(stdin);
			if(strcmp(answer, "s") == 0 || strcmp(answer, "S") == 0 ){
				printf(GREEN"%s\n"RESET, "Grazie per aver utilizzato la nostra bacheca");
				free(msg);
				exit(0);
			}
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
			fprintf(list_fd, "%s\n", "Premi 'q' per uscire.");
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
				fprintf(list_fd, "\nUtente: ");
				fprintf(list_fd, "%s ", announce->user);
				fprintf(list_fd, "ID: ");
				fprintf(list_fd, "%d\n", announce-> id);
				memset(announce, 0, sizeof(struct announce));
			}
			free(announce);
			if(fclose(list_fd) != 0){
				printf(RED"%s\n"RESET, "Error in fclose");
				exit(0);
			}
			if(system("less -r .list.txt") == -1){
				printf("%s\n", "La lista di annunci è stata copiata nel file '.list.txt'");
			}
			if(count){
				printf("\n%ld annunci disponibili al momento\n", count);
			}else{
				printf("%s\n", "Nessun annuncio disponibile");
			}
		}else if(strcmp("post", command) == 0){
			if(login == 1){
				char title[title_size +1];
				char text[text_size + 1];
				memset(title, 0, title_size + 1);
				memset(text, 0, text_size + 1);
				//Acquiring title
				printf("%s\n", "Inserisci il titolo dell'annuncio (Premi Ctrl+G poi Invio per confermare):");
				scanf("%[^\007]", title);				
				fflush(stdin);
				if(strlen(title) >= title_size){
					printf(YELLOW"%s%d%s\n"RESET, "Il titolo deve essere lungo al più ", title_size -1, " caratteri");
					continue;
				}
				//Acquiring text
				printf("%s\n", "Inserisci il testo dell'annuncio(Premi Ctrl+G poi Invio per confermare):");
				scanf("%[^\007]", text);
				fflush(stdin);
				if(strlen(text) >= text_size){
					printf(YELLOW"%s%d%s\n"RESET, "L'annuncio deve essere lungo al più ", text_size -1, " caratteri");
					continue;
				}
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
					printf(GREEN"%s\n"RESET, "Annuncio pubblicato con successo");
				}else if(result == 0){
					printf(YELLOW"%s\n"RESET, "Annuncio non pubblicato");
				}else{
					printf(RESET"%s\n"RESET, "Errore di sincronizzazione con il server, provare a rilanciare il programa");
				}
				memset(title, 0, MAXLINE);
				memset(text, 0, MAXLINE);
			}else{
				printf(YELLOW"%s\n"RESET, "Comando non disponibile: devi effettuare il login prima di poter utilizzare questa funzione");
			}
		}else if(strcmp("delete", command) == 0){
			if(login == 1){
				char identifier[MAXLINE];
				memset(identifier, 0, MAXLINE);
				printf("%s\n", "Inserisci l'ID dell'annuncio:");
				scanf("%[^\n]", identifier);
				fflush(stdin);
				char* rest;
				int id = strtol(identifier, &rest, 10);
				if(strcmp("", rest) != 0){
					printf(YELLOW"ID inserito non valido\n"RESET);
					continue;
				}
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
					if(delete == 0){
						printf(YELLOW"%s\n"RESET, "ID non valido");
					}else if(delete == 1){
						printf(GREEN"%s\n"RESET, "Annuncio cancellato");
					}else if(delete == -1){
						printf(RED"%s\n"RESET, "L'annuncio non è stato pubblicato da questo account");
					}else {
						printf(RED"%s\n"RESET, "Risposta del server errata");
					}

				}else{
					printf(YELLOW"ID inserito non valido\n"RESET);
				}
			}else{
				printf(YELLOW"%s\n"RESET, "Comando non disponibile: devi effettuare il login prima di poter utilizzare questa funzione");
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
				fprintf(list_fd, "%s\n", "Premi 'q' per uscire.");
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
				if(system("less -r .list.txt") == -1){
					printf("%s\n", "La lista di annunci è stata copiata nel file '.list.txt'");
				}
				if(count){
					printf("\n%ld annunci pubblicati\n", count);
				}else{
					printf("%s\n", "Nessun annuncio pubblicato");
				}
			}else{
				printf(YELLOW"%s\n"RESET, "Comando non disponibile: devi effettuare il login prima di poter utilizzare questa funzione");
			}
		}else if(strcmp("logout", command) == 0){
			if(login){
				char answer[MAXLINE];
				memset(answer, 0 , MAXLINE);
				printf("%s", "Sei sicuro di voler fare il logout? [S/n]: ");
				scanf("%[^\n]", answer);
				fflush(stdin);
				if(strcmp(answer, "s") == 0 || strcmp(answer, "S") == 0 ){
					strcpy(msg -> cmd, command);
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
						login = 0;
						strcpy(user, "");
						printf(GREEN"%s\n"RESET, "Utente disconnesso");
					}else{
						printf("%s\n", "Errore nella risposta del server");
						exit(0);
					}
				}
			}else{
				printf(YELLOW"%s\n"RESET, "Non hai ancora effettuato il login");
			}
		}else{
			printf(RED"%s\n"RESET, "Comando non riconosciuto");
		}
	}
}