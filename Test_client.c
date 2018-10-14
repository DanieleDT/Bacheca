#include <stdio.h>

#define fflush(stdin) while(getchar() != '\n')

void main(){
	char size[512];
	printf("%s\n", "Digita il numero di client da avviare");
	scanf("%[^\n]", command);
	fflush(stdin);
	char* rest;
	int value = strtol(identifier, &rest, 10);
	if(strcmp(identifier, rest) == 0){
		printf("Input inserito non valido\n");
	}
	if (value <= 0){
		printf("Input inserito non valido\n");
	}
	pid_t pid;
	for(int i = 0; i < value; i++){
		pid = fork();
		if(pid == 0){
			
		}
	}
}