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

void handle_open(struct sandbox *sb, unsigned long long dataAddr, unsigned long long flagAddress);
void handle_exec(struct sandbox *sb, struct user_regs_struct *regs);
void read_config_file(char *);
void parse_arguments(int, char **, struct parsed_params *);
void handle_no_permission(struct sandbox *sb, int modeToHandle, unsigned long long address);
void handle_no_permission_directory(struct sandbox *sb, int modeToHandle, unsigned long long address, int);
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

      if(isDirectory(f[i].fileName)) {
        length = strlen(f[i].fileName);
        if(f[i].fileName[length-1] != '/') {
          f[i].fileName[length] = '/';
          f[i].fileName[length+1] = '\0';
        }
      }
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
    char * str = (char *) malloc (sizeof(char) * (strlen(temp) + 500));
    realpath(temp, str);
  
    //strcpy(str, temp);
    //printf("The expanded file path of size: %d is : %s\n", strlen(str), str );
    return str;
}

void handle_no_permission(struct sandbox *sb, int modeToHandle, unsigned long long address) {
    //sandb_kill(sb, filePath);
    char cwd[1024];
    char restrictedFilePath[1024];
    char *fileName = create_temp_files(modeToHandle);
    int i=0;
    sprintf(restrictedFilePath, "%s/%s", "/tmp", fileName);
    //printf("restrictedFilePath: %s\n", restrictedFilePath);
    for(i=0; i<strlen(restrictedFilePath); i++) {
      ptrace(PTRACE_POKEDATA, sb->child, address + i, restrictedFilePath[i]);
    }
    // printf("New registry value: %s\n", getFilePathFromSysCall(sb->child, address));
}

void handle_no_permission_directory(struct sandbox *sb, int modeToHandle, unsigned long long address, int mode) {
    //sandb_kill(sb, filePath);
    char cwd[1024];
    char restrictedFilePath[1024];
    char *fileName = create_temp_files(modeToHandle);
    int i=0;

    sprintf(restrictedFilePath, "%s/%s/a", "/tmp", fileName);
    if(mode==1){
      mkdir(restrictedFilePath, 000);      
    }
    for(i=0; i<strlen(restrictedFilePath); i++) {
      ptrace(PTRACE_POKEDATA, sb->child, address + i, restrictedFilePath[i]);
    }
    // printf("New registry value: %s\n", getFilePathFromSysCall(sb->child, address));
}

/*void handle_no_permission_for_exec(struct sandbox *sb, int modeToHandle, unsigned long long address) {
    //sandb_kill(sb, filePath);
    char cwd[1024];
    char *fileName = create_temp_files(modeToHandle);
    printf("File name returned from funct : %s\n", fileName);
    int i=0;
    union aa {
      long int addr;
      char c[8];
    }data;

    union u {
            long val;
            char chars[8];
    }insert;

    for(i=0; i<strlen(fileName)/8; i++) {
      memcpy(insert.chars, fileName, 8);
      ptrace(PTRACE_POKEDATA, sb->child, address + i*8, insert.val);
      fileName += 8;
    }
    int j=strlen(fileName) % 8;
    if(j != 0) {
        memcpy(insert.chars, fileName, j);
        ptrace(PTRACE_POKEDATA, sb->child, address + i * 8, insert.val);
        ptrace(PTRACE_POKEDATA, sb->child, address + (i * 8) + j, '\0');
    }
    /*for(i=0; i<strlen(fileName); i++) {
      data.addr = ptrace(PTRACE_PEEKDATA, sb->child, i + address, NULL);
      printf("Going to replace: %ld\n", data.addr);
      ptrace(PTRACE_POKEDATA, sb->child, address + i, fileName[i]);
      data.addr = ptrace(PTRACE_PEEKDATA, sb->child, address+i, NULL);
      printf("replaced with: %c\n", data.c);
    }

    //ptrace(PTRACE_POKEDATA, sb->child, address + i, '\0');
}*/
 
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
    if(fnmatch(f[index].fileName, str, FNM_NOESCAPE|FNM_PATHNAME) == 0) {
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

void handle_open(struct sandbox *sb, unsigned long long dataAddr, unsigned long long flagAddress) {
    
    char *str = getFilePathFromSysCall(sb->child, dataAddr);
    if(isDirectory(str)) {
      if(str[strlen(str)-1] != '/') {
        str[strlen(str)] = '/';
        str[strlen(str) +1] = '\0';
      }
    }
    // printf("The string for open from REGISTRY: %s\n", str);

    int flag = flagAddress;
    int index;
    // directory flag enabled
    if((flag & 65536) == O_DIRECTORY) {
        //printf("Direcotyr...\n");
        if((flag & 1) == O_RDONLY) {
          if((hasFilePermission(str, READ_MODE)) == 0 && hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0) {
            // printf("No permissions at all...\n");
            handle_no_permission(sb, NO_PERM_DIRECTORY, dataAddr); 
          } else if((hasFilePermission(str, READ_MODE)) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0) {
            //printf("No permissions for read and execute...\n");
            handle_no_permission(sb, WRITE_ONLY_DIRECTORY, dataAddr); 
          } else if((hasFilePermission(str, READ_MODE)) == 0 && hasFilePermission(str, WRITE_MODE) == 0) {
            //printf("No permissions for read and execute...\n");
            handle_no_permission(sb, EXECUTE_ONLY_DIRECTORY, dataAddr); 
          } else if((hasFilePermission(str, READ_MODE)) == 0) {
            //printf("No permissions for read and execute...\n");
            handle_no_permission(sb, WRITE_EXECUTE_DIRECTORY, dataAddr); 
          }
        }

        if((flag & 1) == O_WRONLY || (flag & 64)== O_CREAT) {
          // printf("write and create only...\n");
          if((hasFilePermission(str, READ_MODE)) == 0 && hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0) {
            //printf("No permissions at all...\n");
            handle_no_permission(sb, NO_PERM_DIRECTORY, dataAddr); 
          } else if(hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0)  {
            handle_no_permission(sb, READ_ONLY_DIRECTORY, dataAddr);
          } else if(hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, READ_MODE) == 0)  {
            handle_no_permission(sb, EXECUTE_ONLY_DIRECTORY, dataAddr);
          } else if(hasFilePermission(str, WRITE_MODE) == 0)  {
            handle_no_permission(sb, READ_EXECUTE_DIRECTORY, dataAddr);
          } 
          
        }
        if((flag & 2)== O_RDWR) { 
          // printf("Read and write...\n");
          if((hasFilePermission(str, READ_MODE)) == 0 && hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0) {
            //printf("No permissions at all...\n");
            handle_no_permission(sb, NO_PERM_DIRECTORY, dataAddr); 
          } else if(hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0)  {
            handle_no_permission(sb, READ_ONLY_DIRECTORY, dataAddr);
          } else if(hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, READ_MODE) == 0)  {
            handle_no_permission(sb, EXECUTE_ONLY_DIRECTORY, dataAddr);
          } else if(hasFilePermission(str, READ_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0)  {
            handle_no_permission(sb, WRITE_ONLY_DIRECTORY, dataAddr);
          } else if(hasFilePermission(str, READ_MODE) == 0) {
            handle_no_permission(sb, WRITE_EXECUTE_DIRECTORY, dataAddr);
          } else if(hasFilePermission(str, WRITE_MODE) == 0) {
            handle_no_permission(sb, READ_EXECUTE_DIRECTORY, dataAddr);
          }
        }
    } else {
      // Directory flag disabled...
      if((flag & 1) == O_RDONLY) {
        // Read only block
        // printf("Read only...\n");
          if((hasFilePermission(str, READ_MODE)) == 0 && hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0) {
            // printf("No permissions at all...\n");
            handle_no_permission(sb, NO_PERM_FILE, dataAddr); 
          } else if((hasFilePermission(str, READ_MODE)) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0) {
            //printf("No permissions for read and execute...\n");
            handle_no_permission(sb, WRITE_ONLY_FILE, dataAddr); 
          } else if((hasFilePermission(str, READ_MODE)) == 0 && hasFilePermission(str, WRITE_MODE) == 0) {
            //printf("No permissions for read and execute...\n");
            handle_no_permission(sb, EXECUTE_ONLY_FILE, dataAddr); 
          } else if((hasFilePermission(str, READ_MODE)) == 0) {
            //printf("No permissions for read and execute...\n");
            handle_no_permission(sb, WRITE_EXECUTE_FILE, dataAddr); 
          }
      }

      if((flag & 1) == O_WRONLY || (flag & 64)== O_CREAT) {
        // printf("write and create only...\n");
          if((hasFilePermission(str, READ_MODE)) == 0 && hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0) {
            //printf("No permissions at all...\n");
            handle_no_permission(sb, NO_PERM_FILE, dataAddr); 
          } else if(hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0)  {
            handle_no_permission(sb, READ_ONLY_FILE, dataAddr);
          } else if(hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, READ_MODE) == 0)  {
            handle_no_permission(sb, EXECUTE_ONLY_FILE, dataAddr);
          } else if(hasFilePermission(str, WRITE_MODE) == 0)  {
            handle_no_permission(sb, READ_EXECUTE_FILE, dataAddr);
          } 
        
      }
      if((flag & 2)== O_RDWR) { 
        // printf("Read and write...\n");
          if((hasFilePermission(str, READ_MODE)) == 0 && hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0) {
            //printf("No permissions at all...\n");
            handle_no_permission(sb, NO_PERM_FILE, dataAddr); 
          } else if(hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0)  {
            handle_no_permission(sb, READ_ONLY_FILE, dataAddr);
          } else if(hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, READ_MODE) == 0)  {
            handle_no_permission(sb, EXECUTE_ONLY_FILE, dataAddr);
          } else if(hasFilePermission(str, READ_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0)  {
            handle_no_permission(sb, WRITE_ONLY_FILE, dataAddr);
          } else if(hasFilePermission(str, READ_MODE) == 0) {
            handle_no_permission(sb, WRITE_EXECUTE_FILE, dataAddr);
          } else if(hasFilePermission(str, WRITE_MODE) == 0) {
            handle_no_permission(sb, READ_EXECUTE_FILE, dataAddr);
          }
      }
    }
    
}

void sandb_handle_syscall(struct sandbox *sandb) {
  int i;
  struct user_regs_struct regs;

  if(ptrace(PTRACE_GETREGS, sandb->child, NULL, &regs) < 0)
    err(EXIT_FAILURE, "[SANDBOX] Failed to PTRACE_GETREGS:");

  switch(regs.orig_rax) {

    case SYS_open: {
      handle_open(sandb, regs.rdi, regs.rsi);
      break;
    }
              
    case SYS_openat: {
      handle_open(sandb, regs.rsi, regs.rdx);
      break;
    }
              
    case SYS_execve: {
      char *str = getFilePathFromSysCall(sandb->child, regs.rdi);
      union u {
        long int addr;
        long int addr2[8];
      }data;

      data.addr = ptrace(PTRACE_PEEKDATA, sandb->child, regs.rsi, NULL);
      char *arg = getFilePathFromSysCall(sandb->child, data.addr2[0]);
      /*printf ("1st Argument: %s\n", str);
      printf ("2nd Argument [0]: %s\n", arg);*/
      if(strcmp(str, sandb->progname) != 0) {
          if((hasFilePermission(str, READ_MODE)) == 0 && hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0) {
            // printf("No permissions at all...\n");
            handle_no_permission(sandb, NO_PERM_FILE, regs.rdi); 
            handle_no_permission(sandb, NO_PERM_FILE, data.addr2[0]);
          } else if(hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0)  {
            // printf("No permissions at all...\n");
            handle_no_permission(sandb, READ_ONLY_FILE, regs.rdi);
            handle_no_permission(sandb, READ_ONLY_FILE, data.addr2[0]);
          } else if(hasFilePermission(str, EXECUTE_MODE) == 0 && hasFilePermission(str, READ_MODE) == 0)  {
            handle_no_permission(sandb, WRITE_ONLY_FILE, regs.rdi);
            handle_no_permission(sandb, WRITE_ONLY_FILE, data.addr2[0]);
          } else if(hasFilePermission(str, EXECUTE_MODE) == 0)  {
            handle_no_permission(sandb, READ_WRITE_FILE, regs.rdi);
            handle_no_permission(sandb, READ_WRITE_FILE, data.addr2[0]);
          } 
      }
      break;
    }

    case SYS_mkdir: {
          char *str = getFilePathFromSysCall(sandb->child, regs.rdi);
          if((hasFilePermission(str, READ_MODE)) == 0 && hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0) {
            // printf("No permissions at all...\n");
            handle_no_permission_directory(sandb, NO_PERM_DIRECTORY, regs.rdi, 0); 
          } else if(hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0)  {
            // printf("No permissions at all...\n");
            handle_no_permission_directory(sandb, READ_ONLY_DIRECTORY, regs.rdi, 0);
          } else if(hasFilePermission(str, EXECUTE_MODE) == 0 && hasFilePermission(str, READ_MODE) == 0)  {
            handle_no_permission_directory(sandb, WRITE_ONLY_DIRECTORY, regs.rdi, 0);
          } else if(hasFilePermission(str, READ_MODE) == 0 && hasFilePermission(str, WRITE_MODE) == 0)  {
            handle_no_permission_directory(sandb, EXECUTE_ONLY_DIRECTORY, regs.rdi, 0);
          } else if(hasFilePermission(str, EXECUTE_MODE) == 0)  {
            handle_no_permission_directory(sandb, READ_WRITE_DIRECTORY, regs.rdi, 0);
          } else if(hasFilePermission(str, WRITE_MODE) == 0)  {
            handle_no_permission_directory(sandb, READ_EXECUTE_DIRECTORY, regs.rdi, 0);
          }


      break;
    }

    case SYS_rmdir: {
      char *str = getFilePathFromSysCall(sandb->child, regs.rdi);
      if((hasFilePermission(str, READ_MODE)) == 0 && hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0) {
        // printf("No permissions at all...\n");
        handle_no_permission_directory(sandb, NO_PERM_DIRECTORY, regs.rdi, 1); 
      } else if(hasFilePermission(str, WRITE_MODE) == 0 && hasFilePermission(str, EXECUTE_MODE) == 0)  {
        // printf("No permissions at all...\n");
        handle_no_permission_directory(sandb, READ_ONLY_DIRECTORY, regs.rdi, 1);
      } else if(hasFilePermission(str, EXECUTE_MODE) == 0 && hasFilePermission(str, READ_MODE) == 0)  {
        handle_no_permission_directory(sandb, WRITE_ONLY_DIRECTORY, regs.rdi, 1);
      } else if(hasFilePermission(str, READ_MODE) == 0 && hasFilePermission(str, WRITE_MODE) == 0)  {
        handle_no_permission_directory(sandb, EXECUTE_ONLY_DIRECTORY, regs.rdi, 1);
      } else if(hasFilePermission(str, EXECUTE_MODE) == 0)  {
        handle_no_permission_directory(sandb, READ_WRITE_DIRECTORY, regs.rdi, 1);
      } else if(hasFilePermission(str, WRITE_MODE) == 0)  {
        handle_no_permission_directory(sandb, READ_EXECUTE_DIRECTORY, regs.rdi, 1);
      }
      break;
    }

    default:
            break;
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
   
    /*int retval = ptrace(PTRACE_PEEKUSER, sandb->child, 8*RAX, NULL);
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
  

  sandb_init(&sandb);
  ptrace(PTRACE_SETOPTIONS, sandb.child, 0, PTRACE_O_TRACESYSGOOD);
  
  sandb_run(&sandb);
  clean();
  return EXIT_SUCCESS;
}
