#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

#define SYS_EXIT 0
#define SYS_PRINT 1
#define SYS_MMAP 2
#define SYS_MUNMAP 3
#define SYS_SPAWN 4
#define SYS_GETPID 5
#define SYS_WAIT_PID 6
#define SYS_READ_CHAR 7
#define SYS_UPTIME 8
#define SYS_SLEEP 9
#define SYS_KILL 10
#define SYS_OPEN 11
#define SYS_CLOSE 12
#define SYS_READ 13
#define SYS_GETTIME 14

#endif