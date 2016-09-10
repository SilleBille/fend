#include <sys/ptrace.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <sys/user.h>
#include <asm/ptrace.h>
#include <sys/wait.h>
#include <syscall.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <sys/reg.h>
#include "initheader.h"

#define READ_MODE 0
#define WRITE_MODE 1
#define EXECUTE_MODE 2

struct sandbox {
  pid_t child;
  const char *progname;
};

struct sandb_syscall {
  int syscall;
  void (*callback)(struct sandbox*, struct user_regs_struct *regs);
};

struct parsed_params {
  char *configFileName;
  int errorCode;
};

struct files {
  char mode[4];
  char fileName[1000];
}f[1000];


void handle_open(struct sandbox *sb, struct user_regs_struct *regs);
void handle_openat(struct sandbox *sb, struct user_regs_struct *regs);
void handle_rename(struct sandbox *sb, struct user_regs_struct *regs);
void handle_exec(struct sandbox *sb, struct user_regs_struct *regs);
void read_config_file(char *);
void parse_arguments(int, char **, struct parsed_params *);
void handle_no_permission(struct sandbox *sb, char * filePath, unsigned long long address);
void sandb_kill(struct sandbox *, char *);
int isDirectory(const char *path);



char **argumentsToExec;
int argumentToExecCount;
const char *fname = ".fendrc";
int numberOfEntries = 0;


void read_config_file(char *fileName) {

  FILE *fp;

  fp = fopen (fileName, "r");  
  int i=0;
  char line[1000];
  int flags = 0;
   while(fgets(line, 500, fp) != NULL)
   { 
      strcpy(f[i].mode, strtok (line," "));
      strcpy(f[i].fileName, strtok (NULL, " "));
      int length = strlen(f[i].fileName);

      f[i].fileName[length-1] = '\0';

      // length value decreases by 1;
      /*length = strlen(f[i].fileName);

      if(f[i].fileName[length-1] == '/') {
        f[i].fileName[length] = '*';
        f[i].fileName[length+1] = '\0';
      }*/
      /*if(isDirectory(f[i].fileName)) {
        strcpy(f[i+1].fileName, f[i].fileName);
        length = strlen(f[i+1].fileName);
        if(f[i+1].fileName[length-1] == '/') {
          f[i+1].fileName[length] = '*';
          f[i+1].fileName[length+1] = '\0';
        } else {
          f[i+1].fileName[length] = '/';
          f[i+1].fileName[length+1] = '*';
          f[i+1].fileName[length+2] = '\0';
        }
        strcpy(f[i+1].mode, f[i].mode);
        i++;
      }*/
      i++;
   }
   fclose(fp);  /* close the file prior to exiting the routine */
    int j=0;
   /*for(j=0; j<i; j++) {
      printf("Entries are: %s\n", f[j].fileName);
   }*/
   numberOfEntries = i;
}

char * getFilePathFromSysCall(pid_t pid, unsigned long long int address) {
  char temp[1000];
    union u {
      long int addr;
      char c;
    }data;

    int i = 0;
    while((data.addr = ptrace(PTRACE_PEEKDATA, pid, i + address, NULL)) && data.c != '\0') {
      temp[i] = data.c;
      i++;
    }

    temp[i] = '\0';
    char *str = (char *) malloc (sizeof(char) * strlen(temp));
    strcpy(str, temp);
    return str;
}

void handle_no_permission(struct sandbox *sb, char * fileName, unsigned long long address) {
    //sandb_kill(sb, filePath);
    char cwd[1024];
    char restrictedFilePath[1024];
    union u {
              long int val;
              char chars;
    }data;
    int i=0;
    sprintf(restrictedFilePath, "%s/%s", "/tmp/tempfiles", fileName);
    for(i=0; i<strlen(restrictedFilePath); i++) {
      ptrace(PTRACE_POKEDATA, sb->child, address + i, restrictedFilePath[i]);
    }
    printf("New registry value: %s\n", getFilePathFromSysCall(sb->child, address));
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

struct sandb_syscall sandb_syscalls[] = {
  {SYS_rename,            handle_rename},
  {__NR_write,           NULL},
  {__NR_exit,            NULL},
  {__NR_brk,             NULL},
  {__NR_mmap,            NULL},
  {__NR_access,          NULL},
  {SYS_open,            handle_open},
  {__NR_fstat,           NULL},
  {__NR_close,           NULL},
  {__NR_mprotect,        NULL},
  {__NR_munmap,          NULL},
  {__NR_arch_prctl,      NULL},
  {__NR_exit_group,      NULL},
  {__NR_getdents,        NULL},
  {SYS_execve,          handle_exec},
  {__NR_fadvise64,       NULL},
  {SYS_openat,           handle_openat}
};
int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

int hasFilePermission(char * str, int modeValue) {
  int index;
  int returnValue =1;
  for(index=0; index<numberOfEntries; index++) {
    if(fnmatch(f[index].fileName, str, FNM_NOESCAPE) == 0) {
       returnValue = (f[index].mode[modeValue] == '0') ? 0: 1;
    }
  } 
  //printf("returning value : %d for mode: %d path: %s\n", returnValue, modeValue, str); 
  return returnValue;
}

void sandb_kill(struct sandbox *sandb, char *fileName) {
  fprintf(stderr, "Terminating %s: unauthorized access of %s\n",sandb->progname, fileName);
  kill(sandb->child, SIGKILL);
  wait(NULL);
  exit(EXIT_FAILURE);
}

void handle_open(struct sandbox *sb, struct user_regs_struct *regs) {
    char *str = getFilePathFromSysCall(sb->child, regs->rdi);
    int flag = regs->rsi;
    int index;
    if((flag & 1) == O_RDONLY) {
      // Read only block
      //printf("Read only for: %s-------\n", str);
      if(hasFilePermission(str, READ_MODE) == 0) 
        handle_no_permission(sb, "no_read_file", regs->rdi);
    }

    if((flag & 1) == O_WRONLY || (flag & 64)== O_CREAT) {
      // printf("write and create only...\n");
      if(hasFilePermission(str, WRITE_MODE) == 0)  {
        handle_no_permission(sb, "no_write_directory/write_protected.txt", regs->rdi);
      }
      
    }
    if((flag & 2)== O_RDWR) { 
      // printf("Read and write...\n");
      if(((hasFilePermission(str, WRITE_MODE)) == 0) || (hasFilePermission(str, READ_MODE)) == 0)
        handle_no_permission(sb, "no_write_file", regs->rdi);
    }
    //ptrace(PTRACE_SETREGS, sb->child, NULL, &regs);
}

void handle_openat(struct sandbox *sb, struct user_regs_struct *regs) {
    
    char *str = getFilePathFromSysCall(sb->child, regs->rsi);
    if(isDirectory(str)) {
      if(str[strlen(str)-1] != '/') {
        str[strlen(str)] = '/';
        str[strlen(str) +1] = '\0';
      }
    }
    //printf("The string for openat from REGISTRY: %s\n", str);

    int flag = regs->rdx;
    int index;
    // direcotry flag enabled
    if((flag & 65536) == O_DIRECTORY) {
      //printf("Direcotyr...\n");
      if((flag & 1) == O_RDONLY) {
        if((hasFilePermission(str, READ_MODE)) == 0 || (hasFilePermission(str, EXECUTE_MODE)) == 0) {
          //printf("No Read...\n");
          handle_no_permission(sb, "no_read_directory/", regs->rsi); 
        }
      }

      if((flag & 1) == O_WRONLY || (flag & 64)== O_CREAT) {
        // printf("write and create only...\n");
        if(hasFilePermission(str, WRITE_MODE) == 0 || (hasFilePermission(str, EXECUTE_MODE)) == 0)  {
          printf("No Write...\n");
          handle_no_permission(sb, "no_write_directory", regs->rsi);
        }
        
      }
      if((flag & 2)== O_RDWR) { 
        // printf("Read and write...\n");
        if(((hasFilePermission(str, WRITE_MODE)) == 0) || (hasFilePermission(str, READ_MODE)) == 0)
          handle_no_permission(sb, "no_read_directory", regs->rsi);
      }
    } else {
      // Directory flag disabled...
      if((flag & 1) == O_RDONLY) {
        // Read only block
        // printf("Read only...\n");
        if(hasFilePermission(str, READ_MODE) == 0) 
          handle_no_permission(sb, "no_read_file", regs->rsi);
      }

      if((flag & 1) == O_WRONLY || (flag & 64)== O_CREAT) {
        // printf("write and create only...\n");
        if(hasFilePermission(str, WRITE_MODE) == 0)  {
          handle_no_permission(sb, "no_write_file", regs->rsi);
        }
        
      }
      if((flag & 2)== O_RDWR) { 
        // printf("Read and write...\n");
        if(((hasFilePermission(str, WRITE_MODE)) == 0) || (hasFilePermission(str, READ_MODE)) == 0)
          handle_no_permission(sb, "no_read_file", regs->rsi);
      }
    }

    // ptrace(PTRACE_SETREGS, sb->child, &regs);
    
}

void handle_rename(struct sandbox *sb, struct user_regs_struct *regs) {

}

void handle_exec(struct sandbox *sb, struct user_regs_struct *regs) {
  printf("Need to handle execute\n");
}

void sandb_handle_syscall(struct sandbox *sandb) {
  int i;
  struct user_regs_struct regs;

  if(ptrace(PTRACE_GETREGS, sandb->child, NULL, &regs) < 0)
    err(EXIT_FAILURE, "[SANDBOX] Failed to PTRACE_GETREGS:");

  for(i = 0; i < sizeof(sandb_syscalls)/sizeof(*sandb_syscalls); i++) {
    if(regs.orig_rax == sandb_syscalls[i].syscall) {
      if(sandb_syscalls[i].callback != NULL)
        sandb_syscalls[i].callback(sandb, &regs);
      return;
    }
  }
}

void sandb_init(struct sandbox *sandb) {
  pid_t pid;

  pid = fork();

  if(pid == -1)
    err(EXIT_FAILURE, "Error on fork:");

  if(pid == 0) {

    if(ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0)
      err(EXIT_FAILURE, "Failed to PTRACE_TRACEME:");

    if(execv(argumentsToExec[0], argumentsToExec) < 0)
      err(EXIT_FAILURE, "Failed to execv:");

  } else {
    sandb->child = pid;
    sandb->progname = argumentsToExec[0];
    wait(NULL);
  }
}

int wait_for_syscall(pid_t child) {
    int status;
    while (1) {
        ptrace(PTRACE_SYSCALL, child, 0, 0);
        waitpid(child, &status, 0);
         if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80)
            return 0;
        else if(WIFSTOPPED(status) && WSTOPSIG(status)) {
            return 1;
        }
        if (WIFEXITED(status)) {
          return 1;
        }
        /*if(WIFSIGNALED(status)) {
          return 1;
        }

        if(WCOREDUMP(status) ) {
           return 1;
        }*/
    }
}

void sandb_run(struct sandbox *sandb) {
  int status;
  while(1) {
    // Before executing the syscall. 
    if(wait_for_syscall(sandb->child) != 0) break;
    /*int syscall = ptrace(PTRACE_PEEKUSER, sandb->child, 8*ORIG_RAX, NULL);
    fprintf(stderr, "syscall(%d) = ", syscall);*/


    // Do the necessary handling
    sandb_handle_syscall(sandb);

    // After executing the syscall. Monitor the return value
    if(wait_for_syscall(sandb -> child) != 0) break;
   
 /*   int retval = ptrace(PTRACE_PEEKUSER, sandb->child, 8*RAX, NULL);
    fprintf(stderr, "%d\n", retval);*/

    // I can get the return values
  }
  
}

int main(int argc, char **argv) {
  struct sandbox sandb;

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

  read_config_file(configFile);
  create_temp_files();

  sandb_init(&sandb);
  ptrace(PTRACE_SETOPTIONS, sandb.child, 0, PTRACE_O_TRACESYSGOOD);
  
  sandb_run(&sandb);

  return EXIT_SUCCESS;
}
