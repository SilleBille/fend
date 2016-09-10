#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "initheader.h"
#include <fcntl.h>
char directory[] = "/tmp/tempfiles";
char filename[6][30];
int permissionBits[3] = {0333, 0555, 0666};
void create_temp_files() {
	
	strcpy(filename[0], "no_read_file");
	strcpy(filename[1], "no_write_file");
	strcpy(filename[2], "no_exec_file");
	strcpy(filename[3], "no_read_directory");
	strcpy(filename[4], "no_write_directory");
	strcpy(filename[5], "no_execute_directory");
	int i;
	struct stat dir = {0};
	if(stat(directory, &dir) == -1)
	{
	    mkdir(directory, 0777);
	}
	for(i=0; i<3; i++) {
		char *ff = (char *)malloc((strlen(directory) + strlen(filename[i])) * sizeof(char));
		strcpy(ff, filename[i]);
		sprintf(ff, "%s/%s", directory, filename[i]);
		if(access(ff, F_OK ) != 0) {
			int fd = open(ff, O_CREAT);
			if(fd <0) {
			    perror("Error creating my_log file\n");
			    exit(-1);
			} else {
				fchmod(fd, permissionBits[i]);
			}
		} else {
			chmod(ff, permissionBits[i]);
		}

	}
	for(i=3; i<6; i++) {
		char *ff = (char *)malloc((strlen(directory) + strlen(filename[i])) * sizeof(char));
		strcpy(ff, filename[i]);
		sprintf(ff, "%s/%s", directory, filename[i]);
		if(stat(ff, &dir) == -1)
		{
			mkdir(ff, permissionBits[i-3]);
			chmod(ff, permissionBits[i-3]);
		} else {
			chmod(ff, permissionBits[i-3]);
		}
	}

}