#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "initheader.h"
#include <fcntl.h>
char directory[] = "/tmp";
char filename[16][50];
char *ff;
int permissionBits[16] = {0000, 0444, 0222, 0111, 
						 0666, 0555, 0333, 0777,
						 0000, 0444, 0222, 0111, 
						 0666, 0555, 0333, 0777};
char * create_temp_files(int modeOfFileToBeCreated) {
	strcpy(filename[0], "np");
	strcpy(filename[1], "ro");
	strcpy(filename[2], "wo");
	strcpy(filename[3], "xo");
	strcpy(filename[4], "rw");
	strcpy(filename[5], "rx");
	strcpy(filename[6], "wx");
	strcpy(filename[7], "rwx");
	strcpy(filename[8], "np");
	strcpy(filename[9], "ro");
	strcpy(filename[10], "wo");
	strcpy(filename[11], "xo");
	strcpy(filename[12], "rw");
	strcpy(filename[13], "rx");
	strcpy(filename[14], "wx");
	strcpy(filename[15], "rwx");
	// int i;
	struct stat dir = {0};
	if(stat(directory, &dir) == -1)
	{
	    mkdir(directory, 0777);
	}

	switch(modeOfFileToBeCreated) {
		case 0: 
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7: 
				ff = (char *)malloc((strlen(directory) + strlen(filename[modeOfFileToBeCreated])) * sizeof(char));
				// strcpy(ff, filename[i]);
				sprintf(ff, "%s/%s", directory, filename[modeOfFileToBeCreated]);
				if(access(ff, F_OK ) != 0) {
					int fd = open(ff, O_CREAT);
					if(fd <0) {
					    perror("Error creating my_temp file\n");
					    exit(-1);
					} else {
						fchmod(fd, permissionBits[modeOfFileToBeCreated]);
					}
				} else {
					chmod(ff, permissionBits[modeOfFileToBeCreated]);
				}
				break;
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:ff = (char *)malloc((strlen(directory) + strlen(filename[modeOfFileToBeCreated])) * sizeof(char));
				// strcpy(ff, filename[modeOfFileToBeCreated]);
				sprintf(ff, "%s/%s", directory, filename[modeOfFileToBeCreated]);
				if(stat(ff, &dir) == -1)
				{
					mkdir(ff, permissionBits[modeOfFileToBeCreated]);
					chmod(ff, permissionBits[modeOfFileToBeCreated]);
				} else {
					chmod(ff, permissionBits[modeOfFileToBeCreated]);
				}
				break;

	}


	/*for(i=0; i<3; i++) {
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
	}*/

		return filename[modeOfFileToBeCreated];

}

void clean() {
	int i;
	char fp[400];
	for(i=0; i<8; i++) {
		sprintf(fp, "%s/%s",  directory, filename[i]);
		remove(fp);
	}
	for(i=8; i<16; i++) {
		sprintf(fp, "%s/%s",  directory, filename[i]);
		remove(fp);
	}
	chmod("/tmp/a", 0777);
	chmod("/tmp/a/b", 0777);
	remove("/tmp/a/b/");
	remove("/tmp/a/");
	

}