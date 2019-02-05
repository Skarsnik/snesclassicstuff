#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#define _GNU_SOURCE
#define __USE_GNU
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "server.h"

static int    read_memory(pid_t pid, void* start, unsigned int length, void *out)
{
    struct iovec read_from[1];
    struct iovec read_into[1];
    read_from[0].iov_base = start;
    read_from[0].iov_len = length;
    read_into[0].iov_base = out;
    read_into[0].iov_len = length;
    s_debug("Address to read : %" PRIxPTR "\n", (uintptr_t) start);
    ssize_t read_ret = process_vm_readv(pid, read_into, 1, read_from, 1, 0);
    s_debug("lenght read: %zd\n", read_ret);
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
    write_from[0].iov_base = towrite;
    write_from[0].iov_len = size;
    write_into[0].iov_base = addr;
    write_into[0].iov_len = size;
    ssize_t write_ret = process_vm_writev(pid, write_from, 1, write_into, 1, 0);
    s_debug("lenght written: %zd\n", write_ret);
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
    fp = popen(cmd, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error with popen : %s\n", strerror(errno));
        goto exe_error;
    }
    while ((readed = fread(buf, 1, 2048, fp)) > 0)
    {
        s_debug("Writing %zd to socket\n", readed);
        write(client_fd, buf, readed);
    }
    if (readed < 0)
    {
        fprintf(stderr, "Error with fread : %s\n", strerror(errno));
        goto exe_error;
    }
    pclose(fp);
    write(client_fd, "\0\0\0\0", 4);
    s_debug("End command\n");
    return ;
exe_error:
    write(client_fd, "\0\0ERROR\0\0", 9);

}

static void cmd_detached_command(char* cmd_block, int socket_fd)
{
    
}

static void cmd_exec_read_memory(char* cmd_block, int socket_fd)
{
    size_t          addr;
    pid_t           pid;
    unsigned int    size = 0;
    char*           piko;

    if (sscanf(cmd_block, "READ_MEM %d %zx %d", &pid, &addr, &size) < 0)
    {
        fprintf(stderr, "Error with sscanf %s\n", strerror(errno));
        goto read_error;
    }
    piko = (char*) malloc(size);
    int readed_memory = read_memory(pid, (void*) addr, size, piko);
    if (readed_memory <= 0)
    {
        free(piko);
        goto read_error;
    }
    /*s_debug("===");
    for (unsigned int i = 0; i < size; i++)
    {
        s_debug("%02X", piko[i]);
    }
    s_debug("===\n");*/
    write(socket_fd, piko, readed_memory);
    free(piko);
    return ;
read_error:
    if (size == 9)
        write(socket_fd, "\0\0ERROR\0\0\0", 10);
    else
        write(socket_fd, "\0\0ERROR\0\0", 9);
}

static void cmd_exec_write_memory(char* feed, struct client* client)
{
    if (feed != NULL)
    {
        s_debug("Write mem command\n");
        if (sscanf(feed, "WRITE_MEM %d %zx %d", &(client->write_info.pid), &(client->write_info.addr), &(client->write_info.size)) < 0)
        {
            fprintf(stderr, "Error with sscanf %s\n", strerror(errno));
            client->in_cmd = false;
            write(client->socket_fd, "KO\n", 3);
            return ;
        }
        client->write_buffer = (char*) malloc(client->write_info.size);
        client->write_buffer_size = 0;
        /*if (strlen(feed) + 1 != client->pending_size)
        {
            client->write_buffer_size = client->pending_size - (strlen(feed) + 1);
            memcpy(client->write_buffer, client->pending_data + strlen(feed) + 1, client->write_buffer_size);
            goto check_and_write;
        }*/
    } else {
        memcpy(client->write_buffer + client->write_buffer_size, client->pending_data, client->pending_size);
        client->write_buffer_size += client->pending_size;
        goto check_and_write;
    }
    return;
check_and_write:
    if (client->write_buffer_size == client->write_info.size)
    {
        /*s_debug("Write data to memory : %d %zx %d\n===", client->write_info.pid, client->write_info.addr, client->write_info.size);
        for (unsigned int i = 0; i < client->write_buffer_size; i++)
        {
            s_debug("%02X", client->write_buffer[i]);
        }
        s_debug("===\n");*/
        if (write_memory(client->write_info.pid, (void*)client->write_info.addr, client->write_info.size, client->write_buffer) > 0)
            write(client->socket_fd, "OK\n", 3);
        else
            write(client->socket_fd, "KO\n", 3);
        client->in_cmd = false;
        free(client->write_buffer);
        client->write_buffer_size = 0;
    }
}

static void cmd_stream_file(const char* cmd_block, struct client* client)
{
    cmd_block += 12;
    s_debug("Streaming file : %s", cmd_block);
    client->file_to_stream = open(cmd_block, O_RDONLY);
    if (!client->file_to_stream)
    {
        write(client->socket_fd, "KO\n", 3);
        return ;
    }
    add_streaming_fd(client->file_to_stream);
}

static void cmd_get_file(const char* cmd_block, struct client* client)
{
    int     readed;
    char    buf[2056];

    cmd_block += 9;
    s_debug("Getting file : %s", cmd_block);
    int fd = open(cmd_block, O_RDONLY);
    struct stat st;
    if (stat(cmd_block, &st))
    {
        fprintf(stderr, "Can't state file %s\n", cmd_block);
        memset(buf, 0xFF, 10);
        write(client->socket_fd, buf, 10);
        return ;
    }
    for (unsigned int i = 0; i < 10; i++)
        buf[9 - i] = st.st_size >> 8 * i && 0xFF;
    write(client->socket_fd, buf, 10);
    if (!fd)
    {
        write(client->socket_fd, "KO\n", 3);
        return ;
    }
    while (readed = read(fd, &buf, 2056))
    {
        write(client->socket_fd, buf, readed);
    }
    close(fd);
}

static void cmd_put_file(const char* cmd_block, struct client* client)
{
    uint64_t    size;
    char        buf[2056];

    if (cmd_block != NULL)
    {
        int sizesf = sscanf(cmd_block, "PUT_FILE %" PRId64 , &size);
        if (sizesf < 0)
        {
            fprintf(stderr, "Error with sscanf %s\n", strerror(errno));
            return;
        }
        sprintf(buf, "%" PRId64, size);
        int tmp = strlen(buf);
        strcpy(buf, cmd_block + 9 + tmp + 1);
        s_debug("Uploading file %s of %" PRId64 " bytes\n", buf, size);
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        client->upload_file_fd = open(buf, O_WRONLY | O_CREAT, mode);
        if (client->upload_file_fd < 0)
        {
            fprintf(stderr, "Error opening %s - %s\n", buf, strerror(errno));
            return ;
        }
        client->uploaded_file_size = size;
        client->uploaded_size = 0;
    } else {
        s_debug("Writing to file\n");
        client->uploaded_size += client->pending_size;
        write(client->upload_file_fd, client->pending_data, client->pending_size);
        if (client->uploaded_size == client->uploaded_file_size)
        {
            s_debug("End writing to file\n");
            close(client->upload_file_fd);
            client->in_cmd = false;
        }
    }
}

static void cmd_exec_detached_command(const char* cmd_block, struct client* client)
{
    char*   cmd;
    cmd_block += 13;
    cmd = malloc(strlen(cmd_block) + 2);
    strcpy(cmd, cmd_block);
    cmd[strlen(cmd_block)] = '&';
    s_debug("Runing detached %s\n", cmd);
    system(cmd);
}

void    process_command(struct client* client)
{
    if (client->in_cmd)
    {
        if (client->current_cmd == WRITE_MEM)
        {
            cmd_exec_write_memory(NULL, client);
        }
        if (client->current_cmd == PUT_FILE)
        {
            cmd_put_file(NULL, client);
        }
    } else {
        char* cmd_block = client->pending_data;
        client->pending_data[client->pending_size - 1] = 0;
        client->in_cmd = true;
        s_debug("Command block : %s\n", cmd_block);
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
        if (strncmp(cmd_block, "STREAM_FILE ", 12) == 0)
        {
            client->current_cmd = STREAM_FILE;
            cmd_stream_file(cmd_block, client);
        }
        if (strncmp(cmd_block, "GET_FILE ", 9) == 0)
        {
            client->current_cmd = GET_FILE;
            cmd_get_file(cmd_block, client);
            client->in_cmd = false;
        }
        if (strncmp(cmd_block, "PUT_FILE ", 9) == 0)
        {
            client->current_cmd = PUT_FILE;
            cmd_put_file(cmd_block, client);
        }
        if (strncmp(cmd_block, "CMD_DETACHED ", 13) == 0)
        {
            client->current_cmd = DETACHED_COMMAND;
            cmd_exec_detached_command(cmd_block, client);
            client->in_cmd = false;
        }
    }
}