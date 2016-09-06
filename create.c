#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>

/* 
 * Filename
 * UID
 * GID
 * MODE
 * MTIME
 * SIZE
 * id
 * content
 * 
 */
 
struct file_entry {
	char name[255];
	uid_t uid;
	gid_t gid;
	mode_t mode;
	struct timespec mtime;
	size_t size;
	char ident;
	char* content;
};

void append_arch(char* filename, int file_out)
{
	struct stat st;
	lstat(filename,&st);
	char ident;
	char *buf;
		
	write(file_out, filename, (sizeof(char)*(strlen(filename)+1)));//len + '\0'
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
				append_arch(dir->d_name,file_out);
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

void extract_arch(struct file_entry *file_e)
{
	
}

void list_arch(struct file_entry *file_e)
{
	printf("%c %s %d:%d %d %o %s", file_e->ident, file_e->name, file_e->uid,
			file_e->gid, (int)file_e->size, file_e->mode,
			ctime((time_t*)&file_e->mtime));
			
			//TODO: format
}

int get_entry(struct file_entry * file_e, int *fd)
{
	int num_read;
	char* buf;
	
	buf = file_e->name;
	while( (num_read = read(*fd, buf, 1)) > 0 ){
		if(*buf=='\0') {
			break;
		}
		buf++;
	}
	
	if (num_read <= 0) {
		return 1;
	}
	
	buf = (char*) &file_e->uid;
	if ( (num_read = read(*fd, buf, sizeof(uid_t))) <= 0 ) {
		exit(EXIT_FAILURE);
	}
	
	buf = (char*) &file_e->gid;
	if ( (num_read = read(*fd, buf, sizeof(gid_t))) <= 0 ) {
		exit(EXIT_FAILURE);
	}
	
	buf = (char*) &file_e->mode;
	if ( (num_read = read(*fd, buf, sizeof(mode_t))) <= 0 ) {
		exit(EXIT_FAILURE);
	}
	
	buf = (char*) &file_e->mtime;
	if ( (num_read = read(*fd, buf, sizeof(struct timespec))) <= 0 ) {
		exit(EXIT_FAILURE);
	}
	
	buf = (char*) &file_e->size;
	if ( (num_read = read(*fd, buf, sizeof(size_t))) <= 0 ) {
		exit(EXIT_FAILURE);
	}
	
	buf = (char*) &file_e->ident;
	if ( (num_read = read(*fd, buf, sizeof(char))) <= 0 ) {
		exit(EXIT_FAILURE);
	}
	
	
	if (file_e->size > 0 && S_IFDIR != (file_e->mode & S_IFMT)) {
		file_e->content = malloc(file_e->size);
		buf = file_e->content;
		if( (num_read = read(*fd, buf, file_e->size)) <= 0 ) {
			exit(EXIT_FAILURE);
		}
	}
	
	return 0;
}

void read_arch(int *fd,
				void (*func)(struct file_entry*))
{
	struct file_entry file_e;
	
	while(get_entry(&file_e, fd) == 0) {
		(*func)(&file_e);
	}
	
}
