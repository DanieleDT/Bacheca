#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string.h>

#include "Persistence.h"

#define user_file "./user"
#define announce_file "./announce"

struct announce {
	char oggetto[250];
	char testo[250];
	char utente[250];
	int id;
};

char insert_user(char *username, char *password){
	//ritorna 1 in caso di inserimento, 0 altrimenti
	if(strlen(username) >= 250 | strlen(password) >= 250){
		printf("%s\n", "Username o password troppo lunghi");
		return 0;
	}
	int tempfd = open(user_file, O_CREAT, 0666);
	if(tempfd == -1){
		printf("%s\n", "Error in open");
		exit(0);
	}
	if(close(tempfd) == -1){
		printf("Error in close\n");
		exit(0);
	}
	FILE* fd = fopen(user_file, "r+");
	if(fd == NULL){
		printf("Error in fopen\n");
		exit(0);
	}
	char user[250];
	char pass[250];
	while(fscanf(fd, "%s %s\n", user, pass) != EOF){
		if(strcmp(username, user) == 0){
			return 0;
		}
	}
	fprintf(fd, "%s", username);
	fprintf(fd, "%s", " ");
	fprintf(fd, "%s\n", password);
	if(fclose(fd) != 0){
		printf("%s\n", "Error in fclose");
		exit(0);
	}
	return 1;
}

char login(char * username, char * password){
	//ritorna 1 in caso di login, 0 altrimenti
	int tempfd = open(user_file, O_CREAT, 0666);
	if(tempfd == -1){
		printf("%s\n", "Error in open");
		exit(0);
	}
	if(close(tempfd) == -1){
		printf("Error in close\n");
		exit(0);
	}
	FILE* fd = fopen(user_file, "r");
	if(fd == NULL){
		printf("Error in fopen\n");
		exit(0);
	}
	char user[250];
	char pass[250];
	while(fscanf(fd, "%s %s\n", user, pass) != EOF){
		if(strcmp(username, user) == 0){
			if(strcmp(password, pass) == 0){
				return 1;
			}
		}
	}
	if(fclose(fd) != 0){
		printf("%s\n", "Error in fclose");
		exit(0);
	}
	return 0;
}

char insert_announce(char* oggetto, char* testo, char* utente){
	//ritorna 1 in caso di successo, 0 altrimenti
	if(strlen(oggetto) >= 250 | strlen(testo) >= 250){
		printf("Input non valido\n");
		return 0;
	}
	int tempfd = open(announce_file, O_CREAT, 0666);
	if(tempfd == -1){
		printf("%s\n", "Error in open");
		exit(0);
	}
	if(close(tempfd) == -1){
		printf("Error in close\n");
		exit(0);
	}
	FILE* fd = fopen(announce_file, "r+");
	if(fd == NULL){
		printf("Error in fopen\n");
		exit(0);
	}
	int offset;
	rewind(fd);
	if(fseek(fd, 0, SEEK_END) == -1){
		printf("Error in fseek \n");
		exit(0);
	}
	offset = ftell(fd);
	//printf("offset %d\n", offset);
	struct announce new_announce;

	if(offset > 0){
		struct announce last_announce;
		offset -= sizeof(struct announce);
		if(fseek(fd, offset, SEEK_SET) == -1){
			printf("Error in lseek 3\n");
			exit(0);
		}
		int size = sizeof(last_announce);
		fread(&last_announce, size, 1, fd);
		//printf("oggetto: %s id: %d\n", last_announce.oggetto, last_announce.id);
		new_announce.id = (last_announce.id) +1;
		strcpy(new_announce.oggetto, oggetto);
		strcpy(new_announce.testo, testo);
		strcpy(new_announce.utente, utente);
		fwrite(&new_announce, sizeof(new_announce), 1, fd);
	}else{
		// file vuoto
		printf("%s\n", "file vuoto");
		new_announce.id = 0;
		strcpy(new_announce.oggetto, oggetto);
		strcpy(new_announce.testo, testo);
		strcpy(new_announce.utente, utente);
		fwrite(&new_announce, sizeof(new_announce), 1, fd);
	}
	if(fclose(fd) != 0){
		printf("%s\n", "Error in fclose");
		exit(0);
	}
	return 1;
}

char delete_announce(){

}

/*void main(){
	insert_user("a", "a");
}*/