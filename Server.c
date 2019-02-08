#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <signal.h>

#include "Persistence.h"

#define MAX_THREAD 10
#define MAX_JOBS 10
#define SERVER_PORT 5195
#define IP "127.0.0.1"
#define BACKLOG 10

// free threads waiting on accept
int pool_threads = 0;
int listen_socket;

pthread_mutex_t mux_prethread = PTHREAD_MUTEX_INITIALIZER;

void *thread_job(void *param){
	int jobs_completed = 0;
	while(1){
		if(jobs_completed >= MAX_JOBS){
			// after MAX_JOBS requests handled the thread exits
			pthread_exit(0);
		}
		pthread_mutex_lock( &mux_prethread);
		if(pool_threads >= MAX_THREAD){
			//thread not needed, pool already full
			pthread_mutex_unlock( &mux_prethread);
			pthread_exit(0);
		}else{
			//pool not full
			pool_threads++;
			pthread_mutex_unlock( &mux_prethread);
		}
		struct sockaddr_in client;
		int size = sizeof(client);
		// waiting on the socket for a request from a client
		int job_socket = accept(listen_socket,(struct sockaddr *)&client, &size); 
		if(job_socket == -1){
			printf(RED"%s\n"RESET, "Error in accept");
			pthread_exit(0);
		}
		//accept
		pthread_mutex_lock( &mux_prethread);
		//thread busy handling the request
		pool_threads--;
		pthread_mutex_unlock(&mux_prethread);

		//handling the request
		char loggedin = 0;
		char user[user_size];
		memset(user, 0, user_size);
		struct msg *msg = malloc(sizeof(struct msg));
		memset(msg, 0, sizeof(struct msg));
		if(msg == NULL){
			printf(RED"%s\n"RESET, "Error in msg malloc");
			pthread_exit(0);
		}
		while(1){ 
			memset(msg, 0, sizeof(struct msg));
			if(recv(job_socket, msg, sizeof(struct msg), 0) <= 0){
				//client has terminated
				break;
			}
			if(strcmp(msg -> cmd, "login") == 0){
				if(loggedin == 0){
					if(login(msg -> user, msg -> id_pass)){
						loggedin = 1;
						strcpy(user, msg -> user);
					}else{
						loggedin = 0;
					}
					if(send(job_socket, &loggedin, sizeof(char), 0) == -1){
						printf(RED"%s\n"RESET, "Error in send");
						pthread_exit(0);
					}
				}else{
					char already_logged = -1;
					if(send(job_socket, &already_logged, sizeof(char), 0) == -1){
						printf(RED"%s\n"RESET, "Error in send");
						pthread_exit(0);
					}
				}
			}else if(strcmp(msg -> cmd, "register") == 0){
				char result = insert_user(msg -> user, msg -> id_pass);
				if(send(job_socket, &result, sizeof(char), 0) == -1){
					printf(RED"%s\n"RESET, "Error in send");
					pthread_exit(0);	
				}
			}else if(strcmp(msg -> cmd, "post") == 0){
				char result = -1;
				//sends client: 1 success, 0 failure, -1 not logged in
				if(loggedin == 1){
					result = (char)insert_announcement(msg -> user, msg -> title , msg -> text);
				}
				if(send(job_socket, &result, sizeof(char), 0) == -1){
					printf(RED"%s\n"RESET, "Error in send");
					pthread_exit(0);
				}
			}else if(strcmp(msg -> cmd, "list") == 0){
				struct node* head = list();
				while(1){
					if(head != NULL){
						if(head -> announce == NULL){
							printf(RED"%s\n"RESET, "ERROR: ANNOUNCE NULL");
							pthread_exit(0);
						}
						if(head -> announce -> id == 0){
							printf(RED"\n%s\n"RESET, "ERROR: ID = 0");
						}
						if(send(job_socket, (head->announce), sizeof(struct announce), MSG_MORE) != sizeof(struct announce)){
							printf(RED"%s\n"RESET, "Error in send");
							pthread_exit(0);	
						}
						head = head -> next;
					}else{
						break;
					}
				}
				unlock();
				struct announce* end = malloc(sizeof(struct announce));
				memset(end, 0, sizeof(struct announce));
				if(end == NULL){
					printf(RED"%s\n"RESET, "Error in malloc");
					pthread_exit(0);
				}
				end -> id = -1;
				if(send(job_socket, end, sizeof(struct announce), 0) == -1){
					printf(RED"%s\n"RESET, "Error in send");
					pthread_exit(0);
				}
				free(end);
			}else if(strcmp(msg -> cmd, "mylist") == 0){
				if(loggedin == 1){
					if(strcmp(user, msg -> user) == 0){
						struct node* head = list();
						while(1){
							if(head != NULL){
								if(head -> announce == NULL){
									printf(RED"%s\n"RESET, "Error: announce NULL");
									pthread_exit(0);
								}
								if(head -> announce -> id == 0){
									printf(RED"\n%s\n"RESET, "Error: ID = 0");
								}
								if(strcmp(head -> announce -> user, user) == 0){
									if(send(job_socket, (head->announce), sizeof(struct announce), MSG_MORE) != sizeof(struct announce)){
										printf(RED"%s\n"RESET, "Error in send");
										pthread_exit(0);	
									}
								}
								head = head -> next;
							}else{
								break;
							}
						}
						unlock();
						struct announce* end = malloc(sizeof(struct announce));
						memset(end, 0, sizeof(struct announce));
						if(end == NULL){
							printf(RED"%s\n"RESET, "Error in malloc");
							pthread_exit(0);
						}
						end -> id = -1;
						if(send(job_socket, end, sizeof(struct announce), 0) == -1){
							printf(RED"%s\n"RESET, "Error in send");
							pthread_exit(0);
						}
						free(end);
					}
				}
			}else if(strcmp(msg -> cmd, "delete") == 0){
				char result = delete_announcement(msg -> id_pass, msg -> user);
				if(send(job_socket, &result, sizeof(char), 0) == -1){
					printf(RED"%s\n", "Error in send"RESET);
					pthread_exit(0);
				}
			}else if(strcmp(msg -> cmd, "logout") == 0){
				loggedin = 0;
				strcpy(user, "");
				char result = 1;
				if(send(job_socket, &result, sizeof(char), 0) == -1){
					printf(RED"%s\n"RESET, "Error in send");
					pthread_exit(0);
				}
			}else{
				printf(RED"%s\n"RESET, "Unknown command");
			}
			
		}
		free(msg);
		jobs_completed++;
	}
}

int main_job(){
	pthread_detach(pthread_self());
	int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
	printf(GREEN"%s\n"RESET,"Bootstrap completed: Server online");
	printf("\n");
	pthread_t tid;
	int core = 0;
	while(1){
		pthread_mutex_lock(&mux_prethread);
		if(pool_threads <  MAX_THREAD/2){
			if(pthread_create(&tid, NULL, *thread_job, NULL) != 0){
				printf(RED"Error in pthread_create\n"RESET);
				pthread_mutex_unlock(&mux_prethread);
				continue;
			}
			if(pthread_detach(tid) != 0){
				printf("%s\n", "Error in pthread_detach");
			}
			//each thread is assigned to a different core, to balance the load
			core = (++core)%(num_cores);
			cpu_set_t cpuset;
			CPU_ZERO(&cpuset);
			CPU_SET(core, &cpuset);
			pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
		}
		pthread_mutex_unlock(&mux_prethread);
	}
}

int main(int argc, char* argv[]){
	#ifdef SQLITE_FILE
	printf("%s\n", "sql");
	#endif
	printf(BLUE "PID: %d\n" RESET, (int)getpid());
	pthread_t tid;
	struct sockaddr_in addr;
	struct  protoent *ptrp;
	if ((ptrp = getprotobyname("tcp")) == NULL) {
		printf(RED"cannot map \"tcp\" to protocol number"RESET);
		return -1;
	}
	listen_socket = socket(AF_INET, SOCK_STREAM, ptrp->p_proto);
	if(listen_socket == -1){
		printf(RED"%s\n", "Error in socket"RESET);
		return -1;
	}

	if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0){
		printf(RED"setsockopt(SO_REUSEADDR) failed\n"RESET);
		exit(0);
	}

	memset((void *)&addr, 0, sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(SERVER_PORT);
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	if(bind(listen_socket,(struct sockaddr*)&addr,sizeof(addr)) < 0){
		printf(RED"%s\n"RESET, "Error in bind");
		return -1;
	}
	if(listen(listen_socket, BACKLOG) < 0){
		printf(RED"%s\n"RESET, "Error in listen");
		return -1;
	}
	init_database();
	list_init();
	signal(SIGPIPE, SIG_IGN);
	main_job();
	return -1;
}