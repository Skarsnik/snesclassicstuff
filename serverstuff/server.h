#ifndef H_SERVER_H
#define H_SERVER_H

#include <stdbool.h>
#include <unistd.h>
#define MY_DEBUG 1
#ifdef MY_DEBUG
#define s_debug(...)  printf(__VA_ARGS__)
#else
#define s_debug(...)
#endif

enum commands {
    COMMAND,
    READ_MEM,
    WRITE_MEM,
    STREAM_FILE,
    GET_FILE,
    PUT_FILE,
    DETACHED_COMMAND
};

struct mem_info {
    pid_t   pid;
    size_t  addr;
    unsigned int size;
};

struct client {
    int     socket_fd;
    char    readed_data[2048];
    char    pending_data[2048];
    int     readed_size;
    int     pending_size;
    int     pending_pos;
    int     upload_file_fd;
    int     uploaded_size;
    int     uploaded_file_size;
    bool                in_cmd;
    enum commands       current_cmd;
    char*               write_buffer;
    unsigned int        write_buffer_size;
    struct mem_info     write_info;

    int                 file_to_stream;
};

void    add_streaming_fd(int fd);
void    process_command(struct client* client);
void    preprocess_data(struct client* client);
int     start_server();

#endif