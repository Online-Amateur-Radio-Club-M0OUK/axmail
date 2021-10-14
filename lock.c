
/* lock.c - file locking services */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/file.h>

#include "lock.h"
#include "utils.h"

/* Lock a file descriptor. */

int lock_fd(int fd)
{
	if (flock(fd, LOCK_EX)) {
		syserr("Can't lock fd\n");
		return -1;
	}

	return 0;
}

/* Unlock a file descriptor. */

int unlock_fd(int fd)
{
	if (flock(fd, LOCK_UN)) {
		syserr("Can't unlockfd\n");
		return -1;
	}

	return 0;
}
