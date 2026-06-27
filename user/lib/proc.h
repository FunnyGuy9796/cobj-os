#ifndef PROC_H
#define PROC_H

#include <stdint.h>

uint64_t spawn(const char *name);
uint64_t getpid();
int waitpid(uint64_t pid);
int kill(uint64_t pid);

#endif