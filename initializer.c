#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "initheader.h"
#include <fcntl.h>
char directory[] = "tempfiles";
char filename[3][20];
void create_temp_files() {

	
	strcpy(filename[0], "no_read_file");
	strcpy(filename[1], "no_write_file");
	strcpy(filename[2], "no_exec_file");
	int i;
	struct stat dir = {0};
	if(stat(directory, &dir) == -1)
	{
	    mkdir(directory, 0777);
	    printf("created directory testdir successfully! \n");
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
				fchmod(fd, 00000);
			}
		} else {
			chmod(ff, 00000);
		}

	}
}