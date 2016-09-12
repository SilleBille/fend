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
	strcpy(filename[0], "no_perm_file");
	strcpy(filename[1], "rd_o_file");
	strcpy(filename[2], "wr_o_file");
	strcpy(filename[3], "ex_only_file");
	strcpy(filename[4], "rd_wr_file");
	strcpy(filename[5], "rd_ex_file");
	strcpy(filename[6], "wr_ex_file");
	strcpy(filename[7], "rd_wr_ex_file");
	strcpy(filename[8], "no_perm_dir");
	strcpy(filename[9], "rd_o_dir");
	strcpy(filename[10], "wr_o_dir");
	strcpy(filename[11], "ex_o_dir");
	strcpy(filename[12], "rd_wr_dir");
	strcpy(filename[13], "rd_ex_dir");
	strcpy(filename[14], "wr_ex_dir");
	strcpy(filename[15], "rd_wr_ex_dir");
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
	

}