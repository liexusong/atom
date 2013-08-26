#ifndef __LOCKER_H
#define __LOCKER_H

int locker_open(char *path);
int locker_wrlock(int fd);
int locker_rdlock(int fd);
int locker_unlock(int fd);
void locker_destroy(int fd);

#endif
