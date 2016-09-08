#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	char line[100];
	FILE *fp = fopen(argv[1], "r");
	while(fgets(line, 500, fp) != NULL)
   { 
      printf("%s\n", line);
   }
   fclose(fp); 
   return 0;
}