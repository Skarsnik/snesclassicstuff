#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE
#define __USE_GNU
#include <sys/uio.h>

#include "server.h"

static int    read_memory(pid_t pid, void* start, unsigned int length, void *out)
{
    struct iovec read_from[1];
    struct iovec read_into[1];
    read_from[0].iov_base = start;
    read_from[0].iov_len = length;
    read_into[0].iov_base = out;
    read_into[0].iov_len = length;
    s_debug("Address to read : %zu - %zx\n", start, start);
    ssize_t read_ret = process_vm_readv(pid, read_into, 1, read_from, 1, 0);
    s_debug("lenght read: %d\n", read_ret);
    if (read_ret < 0)
    {
        fprintf(stderr, "Error reading the process memory: %s\n", strerror(errno));
        return -1;
    }
    return read_ret;
}

static int  write_memory(pid_t pid, void* addr, unsigned int size, void *towrite)
{
    struct iovec write_from[1];
    struct iovec write_into[1];
    write_from[0].iov_base = (void*) towrite;
    write_from[0].iov_len = size;
    write_into[0].iov_base = (void*) addr;
    write_into[0].iov_len = size;
    return 1;
    ssize_t write_ret = process_vm_writev(pid, write_from, 1, write_into, 1, 0);
    s_debug("lenght written: %d\n", write_ret);
    if (write_ret < 0)
        fprintf(stderr, "Error writing the process memory: %s\n", strerror(errno));
    return write_ret;
}



static void cmd_exec_command(char* cmd, int client_fd)
{
    size_t  readed = 0;
    char    buf[2048];
    FILE*   fp;
    int     pipe_stdout[2];
    int     pipe_stderr[2];
    int     old_stdout;
    int     old_stderr;

    cmd += 4;
    s_debug("Executing command for %d : %s\n", client_fd, cmd);
    pipe(pipe_stdout);
    pipe(pipe_stderr);
    dup2(1, old_stdout);
    dup2(2, old_stderr);
    dup2(pipe_stdout[1], 1);
    dup2(pipe_stderr[1], 2);
    system(cmd);
    close (pipe_stdout[1]);
    close (pipe_stderr[1]);
    dup2(old_stdout, 1);
    dup2(old_stderr, 2);
    s_debug("Writing to socket\n");
    while ((readed = read(pipe_stdout[0], buf, 2048)) > 0)
    {
        s_debug("Writing %d to socket\n", readed);
        write(client_fd, buf, readed);
    }
    s_debug("End command\n");
}

static void cmd_exec_read_memory(char* cmd_block, int socket_fd)
{
    size_t          addr;
    pid_t           pid;
    unsigned int    size;
    char*           piko;

    if (sscanf(cmd_block, "READ_MEM %d %zx %d", &pid, &addr, &size) < 0)
    {
        fprintf(stderr, "Error with sscanf %s\n", strerror(errno));
        return ;
    }
    piko = (char*) malloc(size);
    int readed_memory = read_memory(pid, (void*) addr, size, piko);
    if (readed_memory <= 0)
    {
        write(socket_fd, "\0HELLO\0", 7);
        free(piko);
        return ;
    }
    write(socket_fd, piko, readed_memory);
    free(piko);
    /*char buff[12];
    for (unsigned int i = 0; i < readed_memory; i++)
        {
            sprintf(&buff, "%02X", piko[i]);
            write(socket_fd, buff, 2);
        }
    //write(socket_fd, )*/
}

static void cmd_exec_write_memory(char* feed, struct client* client)
{
    if (feed != NULL)
    {
        s_debug("Write mem command\n");
        if (sscanf(feed, "WRITE_MEM %d %zx %d", &(client->write_info.pid), &(client->write_info.addr), &(client->write_info.size)) < 0)
        {
            fprintf(stderr, "Error with sscanf %s\n", strerror(errno));
            return ;
        }
        client->write_buffer = (char*) malloc(client->write_info.size);
        client->write_buffer_size = 0;
        if (strlen(feed) + 1 != client->pending_size)
        {
            client->write_buffer_size = client->pending_size - (strlen(feed) + 1);
            memcpy(client->write_buffer, client->pending_data, client->write_buffer_size);
            goto check_and_write;
        }
    } else {
        memcpy(client->write_buffer + client->write_buffer_size, client->pending_data, client->pending_size);
        client->write_buffer_size += client->pending_size;
        goto check_and_write;
    }
    return;
check_and_write:
    if (client->write_buffer_size == client->write_info.size)
    {
        s_debug("Write data to memory : %d %zx %d\n", client->write_info.pid, client->write_info.addr, client->write_info.size);
        if (write_memory(client->write_info.pid, client->write_info.addr, client->write_info.size, client->write_buffer) > 0)
            write(client->socket_fd, "OK\n", 3);
        else
            write(client->socket_fd, "KO\n", 3);
        free(client->write_buffer);
        client->write_buffer_size = 0;
    }
}

void    process_command(struct client* client)
{
    /* TODOD Handle multiple command in a single packet? */
    if (client->in_cmd)
    {
        if (client->current_cmd == WRITE_MEM)
        {
            cmd_exec_write_memory(NULL, client);
        }
    } else {
        bool has_endl = false;
        char    cmd_block[2048];
        for (unsigned int i = 0; i < client->pending_size; i++)
        {
            if (client->pending_data[i] == '\n')
                {
                    has_endl = true;
                    memcpy(cmd_block, client->pending_data, i);
                    cmd_block[i] = 0;
                    break;
                }
        }
        if (!has_endl)
            return ;
        client->in_cmd = true;
        if (strncmp(cmd_block, "CMD ", 4) == 0)
        {
            client->pending_size = 0;
            client->current_cmd = COMMAND;
            cmd_exec_command(cmd_block, client->socket_fd);
            client->in_cmd = false;
        }
        if (strncmp(cmd_block, "READ_MEM ", 9) == 0)
        {
            client->pending_size = 0;
            client->current_cmd = READ_MEM;
            cmd_exec_read_memory(cmd_block, client->socket_fd);
            client->in_cmd = false;
        }
        if (strncmp(cmd_block, "WRITE_MEM ", 10) == 0)
        {
            client->current_cmd = WRITE_MEM;
            cmd_exec_write_memory(cmd_block, client);
            client->pending_size = 0;
        }
    }
}