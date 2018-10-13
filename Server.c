#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>

#include "Persistence.h"


#define MAX_THREAD 10
#define MAX_JOBS 10
#define SERVER_PORT 5195
#define IP "127.0.0.1"
#define BACKLOG 10
#define MAX_ARGS 10


// numero di thread liberi per il prethreading
int pool_threads = 0;
int listen_socket;

pthread_mutex_t mux_prethread = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mux_accept = PTHREAD_MUTEX_INITIALIZER;

void *thread_job(void *param){
	int jobs_completed = 0;
	while(1){
		//printf("%d\n", jobs_completed);
		if(jobs_completed >= MAX_JOBS){
			// dopo aver gestito 10 richieste il thread esce
			//printf("%s\n", "thread esce per troppe richieste gestite");
			pthread_exit(0);
		}
		pthread_mutex_lock( &mux_prethread);
		if(pool_threads >= MAX_THREAD){
			pthread_mutex_unlock( &mux_prethread);
			//printf("%s\n", "thread esce prethreading");
			//printf("%d\n", pool_threads);
			pthread_exit(0);
		}else{
			pool_threads++;
			//printf("%d threads aviable (including myself)\n", pool_threads);
			pthread_mutex_unlock( &mux_prethread);
		}
		struct sockaddr_in client;
		int size = sizeof(client);
		int job_socket = accept(listen_socket,(struct sockaddr *)&client, &size);
		// mi metto in attesa di una richiesta con una accept
		if(job_socket == -1){
			printf("%s\n", "Error in accept");
			pthread_exit(0);
		}else{
			//printf("%s\n", "Accept");
		}
		//accept
		pthread_mutex_lock( &mux_prethread);
		pool_threads--;
		//printf("thread pool %d\n", pool_threads);
		pthread_mutex_unlock(&mux_prethread);
		//gestisco la richiesta
		char buf[3*1024];
		char loggedin = 0;
		char user[1024];
		printf("%s\n", "Connessione effettuata");
		while(recv(job_socket, buf, sizeof(buf), 0) >0){ //socket chiusa
			printf("%s\n", buf);
			char* s = " ";
			char* p = (char*)strtok(buf,s);
			int i = 0;
			char* args[MAX_ARGS];
			args[i] = p;
	  		//printf("%s\n", args[0]);

			while(p){
				p = strtok(NULL,s);

				args[++i] = p;
				//printf("%s\n", args[i]);

			}
			//printf("arg 0 %s\n", args[0]);
			//printf("msg ricevuto\n");
			if(strcmp(args[0], "login") == 0){
				if(loggedin == 0){
					char* rest;
					int password = strtol(args[2], &rest, 10);
					if(strcmp(args[2], rest) == 0){
						printf("Password not cifrated\n");
					}else{
						if(login(args[1], password)){
							loggedin = 1;
							strcpy(user, args[1]);
							printf("%s\n", "login effettuato");
						}else{
							loggedin = 0;
							printf("%s\n", "login NON effettuato");
						}
						if(send(job_socket, &loggedin, sizeof(char), 0) == -1){
							printf("%s\n", "Error in send");
							exit(0);
						}
					}
				}else{
					char already_logged = -1;
					if(send(job_socket, &already_logged, sizeof(char), 0) == -1){
						printf("%s\n", "Error in send");
						exit(0);
					}
				}
			}else if(strcmp(args[0], "register") == 0){
				char* rest;
				int password = strtol(args[2], &rest, 10);
				if(strcmp(args[2], rest) == 0){
					printf("Password not cifrated\n");
				}else{
					printf("%s %s %d\n", "REGISTER", args[1], password);
					
					char result = insert_user(args[1], password);
					
					if(send(job_socket, &result, sizeof(char), 0) == -1){
						printf("%s\n", "Error in send");
						exit(0);
					}
				}
			}else if(strcmp("post", args[0]) == 0){
				//sends client: 1 success, 0 failure, -1 not logged in
				if(loggedin == 1){
					if(insert_announcement(args[1], args[2], args[3]) == 1){
						printf("%s\n", "Annuncio inserito");
					}else{
						printf("%s\n", "Annuncio non inserito");
					}
				}else{
					printf("%s\n", "Login non effettuato");
				}
			}else if(strcmp("list", args[0]) == 0){
				struct node* head = list();
				printf("%s\n", "Finito");
				while(head != NULL){
					struct announce* announce = head-> announce;
					
					if(send(job_socket, (head->announce), sizeof(struct announce), 0) == -1){
						printf("%s\n", "Error in send");
						exit(0);
					}
					struct node* old_head = head;
					head = head -> next;
					free(old_head);
				}
				free(head);
				struct announce* end = malloc(sizeof(struct announce));
				end -> id = -1;
				if(send(job_socket, end, sizeof(struct announce), 0) == -1){
					printf("%s\n", "Error in send");
					exit(0);
				}
				free(end);
			}else if(strcmp(args[0], "delete") == 0){
				//printf("%s", "DELETE ");
				char* rest;
				int id = strtol(args[1], &rest, 10);
				if(strcmp(args[1], rest) == 0){
					printf("ID inserito non valido\n");
				}
				char result = delete_announcement(id, args[2]);
				//printf("%d %s\n", id, args[2]);
				if(send(job_socket, &result, sizeof(char), 0) == -1){
					printf("%s\n", "Error in send");
					exit(0);
				}
			}else{
				printf("%s\n", "Comando non riconosciuto");
			}
		}
		jobs_completed++;
		printf("%s\n", "Connessione chiusa");
	}
}

int main_job(){
	printf("%s\n", "Bootstrap completed: Server online");
	pthread_t tid;
	while(1){
		pthread_mutex_lock(&mux_prethread);
		if(pool_threads <  MAX_THREAD/2){
			if(pthread_create(&tid, NULL, *thread_job, NULL) != 0){
				printf("Error in pthread_create\n");
				return -1;
			} 
			//printf("thread %d creato\n", tid);
		}
		pthread_mutex_unlock(&mux_prethread);
	}
}

int main(int argc, char* argv[]){
	pthread_t tid;
	struct sockaddr_in addr;
	struct  protoent *ptrp;
	if ( (ptrp = getprotobyname("tcp")) == NULL) {
		printf("cannot map \"tcp\" to protocol number");
		return -1;
	}
	listen_socket = socket(AF_INET, SOCK_STREAM, ptrp->p_proto);
	if(listen_socket == -1){
		printf("%s\n", "Error in socket");
		return -1;
	}

	if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0){
		printf("setsockopt(SO_REUSEADDR) failed\n");
		exit(0);
	}

	memset((void *)&addr, 0, sizeof(addr));//inizializza socket processo principale
	addr.sin_family=AF_INET;
	addr.sin_port=htons(SERVER_PORT);
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	if(bind(listen_socket,(struct sockaddr*)&addr,sizeof(addr)) < 0){
		printf("%s\n", "Error in bind");
		return -1;
	}
	if(listen(listen_socket, BACKLOG) < 0){
		printf("%s\n", "Error in listen");
		return -1;
	}
	init_database();
	// creo i primi n thread
	for (int i = 0; i < MAX_THREAD; i++){
		if(pthread_create(&tid, NULL, *thread_job, NULL) != 0){
			printf("Error in pthread_create\n");
			return -1;
		} 
	}
	main_job();
	//pause();
	return -1;

}