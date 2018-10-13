#include <string.h>

int hash_code(char* string){
	//simple hash function for protected passwords
	int hash = 0;
	for(int i = 0; i < strlen(string); i++){
		hash = 31 * hash +  string[i];
	}
	return hash;
}
