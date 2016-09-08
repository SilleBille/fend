#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	char **argumentsToExec;
	argumentsToExec = (char **)malloc(argc * sizeof(char *));
	int i, j=0;
	for(i=1; i< argc; i++) {
		argumentsToExec[j] = (char *)malloc(strlen(argv[i]) * sizeof(char));
	    strcpy(argumentsToExec[j++], argv[i]);
	}
	argumentsToExec[j] = NULL;

	printf("Going to execute %s\n", argumentsToExec[0] );
	execv(argumentsToExec[0], argumentsToExec);
}