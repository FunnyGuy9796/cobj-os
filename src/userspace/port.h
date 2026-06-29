#ifndef PORT_H
#define PORT_H

#include <stdint.h>
#include "../multi/process.h"

#define MAX_MSG_SIZE 256

typedef uint64_t port_token_t;

typedef struct msg_node {
    size_t len;
    uint8_t *data;
    struct msg_node *next;
} msg_node_t;

typedef struct port {
    port_token_t token;
    msg_node_t *head;
    msg_node_t *tail;
    process_t *owner;
} port_t;

port_t *port_create();
port_t *port_lookup(port_token_t token);
size_t port_forward(port_token_t dst, void *msg, size_t len);
size_t port_receive(port_token_t token, void *buf, size_t len);
void port_destroy(port_t *port);

#endif