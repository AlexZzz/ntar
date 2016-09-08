#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

/* 
 * Filename
 * UID
 * GID
 * MODE
 * MTIME
 * SIZE
 * id
 * num_files
 * content
 */
 
struct file_entry {
	char name[255];
	uid_t uid;
	gid_t gid;
	mode_t mode;
	struct timespec mtime;
	size_t size;
	char ident;
	int num_files; //if dir
	char* content;
};

//recursively writes files to file_out
void append_arch(char* filename, int file_out)
{
	struct stat st;
	if (lstat(filename,&st) == -1){
		perror(filename);
	}
	char ident;
	char *buf;
	int num_files=0;
	
	if(S_ISSOCK(st.st_mode)) { //don't copy unix sockets
		return;
	}
		
	write(file_out, basename(filename), (sizeof(char)*(strlen(basename(filename))+1)));
	write(file_out, &st.st_uid, sizeof(uid_t));
	write(file_out, &st.st_gid, sizeof(gid_t));
	write(file_out, &st.st_mode, sizeof(mode_t));
	write(file_out, &st.st_mtime, sizeof(struct timespec));
	write(file_out, &st.st_size, sizeof(off_t));
	
	if(S_ISDIR(st.st_mode)) {
		ident='d';
		write(file_out, &ident, sizeof(char));
		
		struct dirent *dir;
		
		DIR* dir_stream = opendir(filename);
		chdir(filename);
		while((dir = readdir(dir_stream)) != NULL) {
			if ( (strcmp(dir->d_name, ".")) != 0 && (strcmp(dir->d_name, "..")) != 0) {
				num_files++;
			}
		}
		write(file_out, &num_files, sizeof(int));// need to know number
		rewinddir(dir_stream);					//before recursive call
		while((dir = readdir(dir_stream)) != NULL) {
			if ( (strcmp(dir->d_name, ".")) != 0 && (strcmp(dir->d_name, "..")) != 0) {
				append_arch(dir->d_name,file_out);
			}
		}
		chdir("..");
		closedir(dir_stream);
		
	} else if(S_ISREG(st.st_mode)) {
		ident = 'f';
		write(file_out, &ident, sizeof(char));
		
		buf = malloc(sizeof(size_t)*256);
		
		int fd = open(filename, O_RDONLY);
		int is_read;
		while((is_read=read(fd, buf, 256)) != 0) {
			write(file_out, buf, is_read);
		}
		close(fd);
		free(buf);
		
	} else if(S_ISLNK(st.st_mode)) {
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
		buf[st.st_size] = '\0';
		write(file_out, buf, st.st_size);
		close(fd);
		free(buf);
		
	} else if(S_ISFIFO(st.st_mode)) { //copy fifo without content
		ident = 'p';
		write(file_out, &ident, sizeof(char));
	}
}

//Gets file entry from *fd and writes to struct file_entry*
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
	
	if (S_ISDIR(file_e->mode)) {
		buf = (char*) &file_e->num_files;
		if ( (num_read = read(*fd, buf, sizeof(int))) <= 0 ) {
			exit(EXIT_FAILURE);
		}
	}
	
	if (file_e->size > 0 && !S_ISDIR(file_e->mode)) {
		file_e->content = malloc(file_e->size);
		buf = file_e->content;
		if( (num_read = read(*fd, buf, file_e->size)) <= 0 ) {
			exit(EXIT_FAILURE);
		}
	}
	
	return 0;
}

//Show files in archive
void list_arch(int *fd)
{
	struct file_entry file_e;
	char* time_str;
	struct passwd *pass;
	struct group *grp;
	
	while(get_entry(&file_e,fd) == 0) {
		time_str = ctime((time_t*)&file_e.mtime);
		time_str[strlen(time_str)-1]='\0'; //remove \n
		pass = getpwuid(file_e.uid);
		grp = getgrgid(file_e.gid);
		printf("\n%c %o %s:%s %d %s %s ",
				file_e.ident,
				file_e.mode,
				pass->pw_name,
				grp->gr_name,
				(int)file_e.size,
				time_str,
				file_e.name);
		if (file_e.ident == 'l') {
			printf(" -> %s", file_e.content);
		}
		printf("\n");
	}
}

//Extract one file
void extract_file(struct file_entry *file_e, int *fd)
{
	if (file_e->ident == 'd') {
		mkdir(file_e->name, file_e->mode);
		chown(file_e->name, file_e->uid, file_e->gid);
		chdir(file_e->name);
		struct file_entry in_dir;
		while(file_e->num_files > 0) {
			if(get_entry(&in_dir,fd) == 0) {
				extract_file(&in_dir, fd);
			}
			file_e->num_files--;
		}
		chdir("..");
	} else if (file_e->ident == 'f') {
		int fd = creat(file_e->name, file_e->mode);
		chown(file_e->name, file_e->uid, file_e->gid);
		write(fd, file_e->content, file_e->size);
		close(fd);
	} else if (file_e->ident == 'l') {
		symlink(file_e->content, file_e->name);
		chown(file_e->name, file_e->uid, file_e->gid);
	} else if (file_e->ident == 'p') {
		mkfifo(file_e->name, file_e->mode);
		chown(file_e->name, file_e->uid, file_e->gid);
	}
}

//Extract full archive
void extract_arch(int *fd)
{
	struct file_entry file_e;
	
	while(get_entry(&file_e,fd) == 0) {
		extract_file(&file_e, fd);
	}	
}
