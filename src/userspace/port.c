#include "port.h"

static port_t *port_table[1024];
static int port_table_count = 0;

static uint64_t rdrand64() {
    uint64_t val;
    uint8_t ok;

    do {
        __asm__ volatile (
            "rdrand %0\n"
            "setc %1\n"
            : "=r"(val), "=qm"(ok)
            :
            : "memory"
        );
    } while (!ok);

    return val;
}

port_t *port_create() {
    uint32_t lo, hi;

    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));

    uint64_t val = ((uint64_t)hi << 32) | lo;

    val ^= val >> 33;
    val *= 0xff51afd7ed558ccdULL;
    val ^= val >> 33;
    val *= 0xc4ceb9fe1a85ec53ULL;
    val ^= val >> 33;

    port_t *port = kmalloc(sizeof(port_t));

    port->token = (port_token_t)val;
    port->head = NULL;
    port->tail = NULL;
    port->owner = curr_thread->process;

    port_table[port_table_count++] = port;

    return port;
}

port_t *port_lookup(port_token_t token) {
    for (int i = 0; i < port_table_count; i++) {
        if (port_table[i] && port_table[i]->token == token)
            return port_table[i];
    }

    return NULL;
}

size_t port_forward(port_token_t dst, void *msg, size_t len) {
    port_t *port = port_lookup(dst);

    if (!port)
        return -1;
    
    msg_node_t *node = kmalloc(sizeof(msg_node_t));

    node->len = len;
    node->data = kmalloc(len);
    node->next = NULL;

    memcpy(node->data, msg, len);

    if (!port->tail)
        port->head = port->tail = node;
    else {
        port->tail->next = node;
        port->tail = node;
    }

    if (port->owner->thread->state == THREAD_BLOCKED)
        port->owner->thread->state = THREAD_READY;
    
    return len;
}

size_t port_receive(port_token_t token, void *buf, size_t len) {
    port_t *port = port_lookup(token);

    if (!port)
        return 0;
    
    while (!port->head) {
        curr_thread->state = THREAD_BLOCKED;

        schedule();
    }

    msg_node_t *node = port->head;

    port->head = node->next;

    if (!port->head)
        port->tail = NULL;
    
    size_t to_copy = node->len < len ? node->len : len;

    memcpy(buf, node->data, to_copy);
    
    kfree(node->data);
    kfree(node);

    return to_copy;
}

void port_destroy(port_t *port) {
    if (port->owner->thread->state == THREAD_BLOCKED)
        port->owner->thread->state = THREAD_READY;
        
    msg_node_t *node = port->head;

    while (node) {
        msg_node_t *next = node->next;

        kfree(node->data);
        kfree(node);

        node = next;
    }

    for (int i = 0; i < port_table_count; i++) {
        if (port_table[i] == port) {
            port_table[i] = NULL;

            break;
        }
    }

    kfree(port);
}