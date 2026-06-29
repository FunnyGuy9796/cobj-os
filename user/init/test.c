#include <printf.h>
#include <port.h>

const char msg[16] = "IPC is working!";

int main(int argc, char **argv) {
    port_token_t port = strtoull(argv[1], NULL, 10);

    printf("test.c: port: %lu\n", port);

    forward_port(port, (void *)msg, 16);
}