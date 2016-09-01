#include <stdio.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <signal.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/reg.h> 

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
int argumentToExecCount;
const char *fname = ".fendrc";

void init(struct sand_box *sb) {

	printf("Parent is going to execute fork...\n");
	pid_t pid = fork();
	if(pid == -1) {
		// Error in forking
		perror("Error forking child: ");
	} 
	if(pid == 0) {
		// Child is being executed
		printf("Child executing...\n");
		if(ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
			// Error in informing ptrace_traceme
			perror("Failed to PTRACE_TRACEME: ");
		}

		printf("child part 2\n");

		if(execv(argumentsToExec[0], argumentsToExec) < 0) {
			// Error in exec 
			perror("Failed in EXECV: ");
		}
	} else {
		// Parent process running
		sb->pid = pid;
		printf("In parent. Child pid created: %d\n", pid);

		// Wait for the child to attach itself to parent.
		wait(NULL);
		printf("Back to parent: \n");
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
	for(i=1; i<argc; i++) {
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
	argumentsToExec[j] = (char *)malloc(sizeof(char));
	argumentsToExec[j] = NULL;
	argumentToExecCount = j; 

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

/* To kill the sandbox if something goes wrong */
void sand_box_kill(struct sand_box *sb) {
	kill(sb->pid, SIGKILL);
	wait(NULL);
	exit(EXIT_FAILURE);
}

/* Run the sandbox and monitor system calls */
void run_sand_box(struct sand_box *sb) {
	int status;
	struct user_regs_struct regs;
	printf("sandbox running...\n");


	if(ptrace(PTRACE_SYSCALL, sb->pid, NULL, NULL) < 0) {
	    if(errno == ESRCH) {
		    waitpid(sb->pid, &status, __WALL | WNOHANG);
		    sand_box_kill(sb);
	    } else {
	      errx(EXIT_FAILURE, "Failed at PTRACE_SYSCALL:");
	    }
  	}
  	printf("Waiting for status change...\n");
  	wait(&status);

  	printf("status changed... Going to registers for pid: %d\n",sb->pid);
  	
  	


    printf("The child made a system call %llu\n", regs.orig_rax);

  	if(WIFEXITED(status)) {
  		// Successfully exiting...
  		printf("Successfully Stopped!\n");
  		exit(EXIT_SUCCESS);
  	} 
  	if(WIFSTOPPED(status)) {
  		printf("STOPPED\n");
  		if(ptrace(PTRACE_GETREGS, sb->pid, NULL, &regs) < 0)
  		 err(EXIT_FAILURE, "Error in getting PTRACE_GETREGS");
  	}
}

int main(int argc, char *argv[]) {

	struct sand_box sb;
	struct parsed_params params;
	char *configFile;
	// Check whether there are atleast 2 arguments
	if(argc < 2) {
		errx(EXIT_FAILURE, "Fend usage: %s -c <configfile> childProcess", argv[0]);
	}

	// Split arguments and find if config file exists
	parse_arguments(argc, argv, &params);

	if(params.errorCode == 1)
		errx(EXIT_FAILURE, "The specified config file doesn't exist\n");
	else if(params.errorCode == 2)
		errx(EXIT_FAILURE, "Must provide a config file.\n");
	else if(params.errorCode == 0) {
		configFile = params.configFileName;
	}
	
	
	init(&sb);
	printf("back to main ...\n");
	while(1) {
		run_sand_box(&sb);
	}
	

	return 0;
}