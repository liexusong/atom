#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "lock.h"


static inline int __do_lock(int fd, int type)
{ 
    int ret;
    struct flock lock;

    lock.l_type = type;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 1;
    lock.l_pid = 0;

    do {
        ret = fcntl(fd, F_SETLKW, &lock);
    } while (ret < 0 && errno == EINTR);

    return ret;
}


int locker_open(char *path)
{
    return open(path, O_RDWR|O_CREAT, 0666);
}


int locker_wrlock(int fd)
{   
    if (__do_lock(fd, F_WRLCK) < 0) {
        return -1;
    }
    return 0;
}


int locker_rdlock(int fd)
{   
    if (__do_lock(fd, F_RDLCK) < 0) {
        return -1;
    }
    return 0;
}


int locker_unlock(int fd)
{   
    if (__do_lock(fd, F_UNLCK) < 0) {
        return -1;
    }
    return 0;
}


void locker_destroy(int fd)
{
    close(fd);
}
