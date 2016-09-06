#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "create.c"

//int create();
//int append();
//int extract();
//int list();

int main(int argc, char*argv[])
{
	if(argc<3) {
		fprintf(stderr, "Usage: %s cf|af|xf|tf archive [filenames...]\n"
			,argv[0]);
		return(1);
	}

	int out_f;
	char* filename;
	struct stat *f_stat=malloc(sizeof(struct stat));
		
	if (strcmp(argv[1], "cf") == 0) {
		
		if ( (out_f = creat(argv[2],0666)) == -1) {
			perror(argv[2]);
			exit(EXIT_FAILURE);
		}
		filename=argv[3];

		append(filename,out_f);

	} else if(strcmp(argv[1], "af") == 0) {
		//check file exists and open with
		//pointer at end and add files
	} else if(strcmp(argv[1], "xf") == 0) {
		//check file exists and extract one-by-one
	} else if(strcmp(argv[1], "tf") == 0) {
		//check file exists and show files and attrs
	} else {
		fprintf(stderr, "Usage: %s cf|af|xf|tf archive [filenames...]\n"
			,argv[0]);
		return(1);
	}
	close(out_f);
	return(0);
}

