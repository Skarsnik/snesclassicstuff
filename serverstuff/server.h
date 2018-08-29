#ifndef H_SERVER_H
#define H_SERVER_H

#include <stdbool.h>
#define MY_DEBUG 1
#ifdef MY_DEBUG
#define s_debug(...)  printf(__VA_ARGS__)
#else
#define s_debug(...)
#endif

enum commands {
    COMMAND,
    READ_MEM,
    WRITE_MEM
};

struct mem_info {
    pid_t   pid;
    size_t  addr;
    unsigned int size;
};

struct client {
    int     socket_fd;
    char    pending_data[2048];
    int     pending_size;
    bool                in_cmd;
    enum commands       current_cmd;
    char*               write_buffer;
    unsigned int        write_buffer_size;
    struct mem_info     write_info;
};

void    process_command(struct client* client);

#endif