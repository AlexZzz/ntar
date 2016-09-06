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

	int fd;
	char* filename;
	struct stat *f_stat=malloc(sizeof(struct stat));
		
	if (strcmp(argv[1], "cf") == 0) {
		
		if ( (fd = creat(argv[2], 0666)) == -1) {
			perror(argv[2]);
			exit(EXIT_FAILURE);
		}
		filename=argv[3];

		append_arch(filename, fd);

	} else if(strcmp(argv[1], "af") == 0) {
		
		if ( (fd = open(argv[2], O_APPEND | O_WRONLY)) == -1 ) {
			perror(argv[2]);
			exit(EXIT_FAILURE);
		}
		filename=argv[3];
		
		append_arch(filename, fd);
	} else if(strcmp(argv[1], "xf") == 0) {
		if ( (fd = open(argv[2], O_RDONLY)) == -1 ) {
			perror(argv[2]);
			exit(EXIT_FAILURE);
		}
	} else if(strcmp(argv[1], "tf") == 0) {
		if ( (fd = open(argv[2], O_RDONLY)) == -1 ) {
			perror(argv[2]);
			exit(EXIT_FAILURE);
		}
		read_arch(&fd, list_arch);
	} else {
		fprintf(stderr, "Usage: %s cf|af|xf|tf archive [filenames...]\n"
			,argv[0]);
		return(1);
	}
	close(fd);
	return(0);
}

