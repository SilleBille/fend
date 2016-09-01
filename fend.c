#include <stdio.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>

struct sand_box {
	pid_t pid;
};

struct read_config {
	char *fileControlled;
	int access[3];
};

struct parsed_params {
	char *configFileName;
	int errorCode;
};

char **argumentsToExec;
const char *fname = ".fendrc";
int argumentToExecCount;

void init(struct sand_box *sb) {

	pid_t pid = fork();


	if(pid == -1) {
		// Error in forking
		perror("Error forking child: ");
	} 
	if(pid == 0) {
		int i;
		for(i=0; i< sizeof(argumentsToExec)/sizeof(char); i++) {
			printf("The argumentsForExec: %s\n", argumentsToExec[i]);
		}
		// Child is being executed
		if(ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
			// Error in informing ptrace_traceme
			perror("Failed to PTRACE_TRACEME: ");
		}
		if(execv(argumentsToExec[0], argumentsToExec) < 0) {
			// Error in exec 
			perror("Failed in EXECV: ");
		}
	} else {
		// Parent process running
		sb->pid = pid;
	}
}

void read_config_file(char *fileName) {
	// TODO: need to read config files and save them
	// and use it to check file access.
}

void parse_arguments(int argc, char *rawArgs[], struct parsed_params *pparams) {
	int i, j=0;
	char *configFileName = NULL;

	argumentsToExec = (char **)malloc(argc * sizeof(char *));
	pparams->errorCode = 0;
	// start from 1 because you don't need the ./fend arg
	for(i=0; i<argc; i++) {
		if(strcmp(rawArgs[i], "-c") == 0) {
			// Config file is specified

			if(access(rawArgs[i+1], F_OK ) == 0 ) {
				configFileName = (char *) malloc(strlen(rawArgs[i+1])* sizeof(char));
				strcpy(configFileName, rawArgs[i+1]);
				pparams->configFileName = configFileName;
			} else {
				pparams->errorCode = 1;
			}
			// we are supposed to skip the next arg because 'fend -c config xxx'
			i++;
			continue;
		}
		argumentsToExec[j] = (char *)malloc(strlen(rawArgs[i]) * sizeof(char));
		strcpy(argumentsToExec[j++], rawArgs[i]);
	}
	argumentToExecCount = j-1; 

	if(configFileName == NULL && pparams->errorCode == 0) {
		// File hasn't been specified
		char *pathToHome = (char *)malloc(300 * sizeof(char));
		// path of getenv("home") is /home/dinesh
		sprintf(pathToHome, "%s/%s", getenv("HOME"), fname);
		//fprintf(pathToHome, "%s")
		if(access(fname, F_OK ) == 0 ) {
    		// .fendrc file exists in current directory
    		configFileName = (char *) malloc(strlen(fname)* sizeof(char));
			strcpy(configFileName, fname);
			pparams->configFileName = configFileName;
		} else if(access(pathToHome, F_OK) == 0) {
			// .fendrc file exists in home directory
			configFileName = (char *) malloc(strlen(pathToHome)* sizeof(char));
			strcpy(configFileName, pathToHome);	
			pparams->configFileName = configFileName;	
		} else {
			 pparams->errorCode = 2;
		}
	}		
}

int main(int argc, char *argv[]) {

	struct sand_box sb;
	struct parsed_params params;
	// Check whether there are atleast 2 arguments
	if(argc < 2) {
		errx(EXIT_FAILURE, "Fend usage: %s -c <configfile> childProcess", argv[0]);
	}

	// Split arguments and find if config file exists
	parse_arguments(argc, argv, &params);

	if(params.errorCode == 1)
		errx(EXIT_FAILURE, "The specified config file doesn't exist");
	else if(params.errorCode == 2)
		errx(EXIT_FAILURE, "The config file isn't specified");
	else if(params.errorCode == 0) {
		char *configFile = params.configFileName;
		printf("config file is: %s\n", configFile);
	}

    

	
	
	init(&sb);
	

	return 0;
}