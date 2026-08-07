
















#ifndef VULCAN_LIBC_STDIO_H
#define VULCAN_LIBC_STDIO_H

#include "vulcan_types.h"

#define VULCAN_STDIN  0
#define VULCAN_STDOUT 1
#define VULCAN_STDERR 2

int open(const char *path, int flags);
void close(int fd);
ssize_t read(int fd, void *buf, usize count);
ssize_t write(int fd, const void *buf, usize count);

int chdir(const char *path);
char *getcwd(char *buf, usize size);
int mkdir(const char *path);
int rmdir(const char *path);
int unlink(const char *path);






#define VULCAN_O_READ    (1 << 0)
#define VULCAN_O_WRITE   (1 << 1)
#define VULCAN_O_CREATE  (1 << 2)

int printf(const char *fmt, ...);





struct vulcan_dirent {
    char name[64]; 




    int is_directory;
};










int vulcan_readdir(const char *path, unsigned long index, struct vulcan_dirent *out);

#endif 
