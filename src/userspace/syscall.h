#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stdbool.h>

#define SYS_EXIT 0
#define SYS_PRINT 1
#define SYS_MMAP 2
#define SYS_MUNMAP 3
#define SYS_SPAWN 4
#define SYS_GETPID 5
#define SYS_WAIT_PID 6
#define SYS_LISTDIR 7
#define SYS_UPTIME 8
#define SYS_SLEEP 9
#define SYS_KILL 10
#define SYS_OPEN 11
#define SYS_CLOSE 12
#define SYS_READ 13
#define SYS_GETTIME 14
#define SYS_LISTPROCS 15
#define SYS_WRITE 16
#define SYS_PORT_CREATE 17
#define SYS_PORT_FORWARD 18
#define SYS_PORT_RECEIVE 19
#define SYS_PORT_DESTROY 20
#define SYS_GET_CWD 21
#define SYS_SET_CWD 22
#define SYS_CLEAR_SCREEN 23

#define USER_SPACE_TOP 0x0000800000000000ULL

#endif