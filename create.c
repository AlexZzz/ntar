#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>

struct file_entry {
	char type[1]; //d, f, p, l
	uid_t uid;
	gid_t gid;
	char filename[255];
	mode_t mode;
	off_t size;
};

void append(char* filename,int file_out)
{
	struct stat st;
	lstat(filename,&st);
	char ident;
	char *buf;
		
	write(file_out, filename, (sizeof(char)*strlen(filename)));
	write(file_out, &st.st_uid, sizeof(uid_t));
	write(file_out, &st.st_gid, sizeof(gid_t));
	write(file_out, &st.st_mode, sizeof(mode_t));
	write(file_out, &st.st_mtime, sizeof(struct timespec));
	write(file_out, &st.st_size, sizeof(off_t));
	
	if(S_IFDIR == (st.st_mode & S_IFMT)) {
		ident='d';
		write(file_out, &ident, sizeof(char));
		
		struct dirent *dir;
		
		DIR* dir_stream = opendir(filename);
		chdir(filename);
		while((dir = readdir(dir_stream)) != NULL) {
			printf("%s\n",dir->d_name);
			if ( (strcmp(dir->d_name, ".")) != 0 && (strcmp(dir->d_name, "..")) != 0) {
				append(dir->d_name,file_out);
			}
		}
		chdir("..");
		closedir(dir_stream);
		
	} else if(S_IFREG == (st.st_mode & S_IFMT)) {
		ident = 'f';
		write(file_out, &ident, sizeof(char));
		
		buf = malloc(sizeof(size_t)*256);
		
		int fd = open(filename, O_RDONLY);
		int is_read;
		while((is_read=read(fd, buf, 256)) != 0) {
			write(file_out, buf, is_read);
		}
		close(fd);
		
	} else if(S_IFLNK == (st.st_mode & S_IFMT)) {
		ident = 'l';
		write(file_out, &ident, sizeof(char));
		
		buf = malloc(st.st_size);
		
		int fd = open(filename, O_RDONLY);
		if (fd == -1) {
			perror(filename);
			exit(1);
		}
		if ((readlink(filename, buf, st.st_size)) == -1) {
			perror(filename);
			exit(1);
		}
		write(file_out, buf, st.st_size);
		close(fd);
		
	} else if(S_IFIFO == (st.st_mode & S_IFMT)) {
		ident = 'p';
		write(file_out, &ident, sizeof(char));
		
	} else if(S_IFSOCK == (st.st_mode & S_IFMT)) {
		ident = 's';
		write(file_out, &ident, sizeof(char));
	}
}
