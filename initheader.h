#define NO_PERM_FILE 0
#define READ_ONLY_FILE 1
#define WRITE_ONLY_FILE 2
#define EXECUTE_ONLY_FILE 3
#define READ_WRITE_FILE 4
#define READ_EXECUTE_FILE 5
#define WRITE_EXECUTE_FILE 6
#define READ_WRITE_EXECUTE_FILE 7
#define NO_PERM_DIRECTORY 8
#define READ_ONLY_DIRECTORY 9
#define WRITE_ONLY_DIRECTORY 10
#define EXECUTE_ONLY_DIRECTORY 11
#define READ_WRITE_DIRECTORY 12
#define READ_EXECUTE_DIRECTORY 13
#define WRITE_EXECUTE_DIRECTORY 14
#define READ_WRITE_EXECUTE_DIRECTORY 15

char * create_temp_files();
void clean();