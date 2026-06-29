#ifndef PORT_H
#define PORT_H

#include <stdint.h>
#include <stddef.h>

typedef uint64_t port_token_t;

port_token_t create_port();
int forward_port(port_token_t dst, void *msg, size_t len);
int receive_port(port_token_t token, void *buf, size_t len);
int destroy_port(port_token_t token);

#endif