#include <stdio.h>
#include <stdlib.h>

void main(){
	char *s;
	while(scanf("%m[^\n]", &s)){
		printf("%s\n", s);
	}
	free(s);
}