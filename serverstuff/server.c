#include <stdbool.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "server.h"

#define MAX_CLIENT 5

struct client   clients[MAX_CLIENT];

static bool add_to_clients(int  client_fd)
{
    for (unsigned int i = 0; i < 5; i++)
    {
        if (clients[i].socket_fd == 0)
        {
            s_debug("Adding new client %d\n", client_fd);
            clients[i].socket_fd = client_fd;
            clients[i].pending_data[0] = 0;
            clients[i].pending_size = 0;
            clients[i].in_cmd = false;
            clients[i].write_buffer = NULL;
            clients[i].write_buffer_size = 0;
            clients[i].file_to_stream = 0;
            clients[i].upload_file_fd = -1;
            clients[i].uploaded_size = 0;
            clients[i].uploaded_file_size = 0;
            return true;
        }
    }
    return false;
}

static struct client* get_client(int client_fd)
{
    for (unsigned int i = 0; i < 5; i++)
    {
        if (clients[i].socket_fd == client_fd)
            return &clients[i];
    }
    return NULL;
}

static void remove_client(int client_fd)
{
    for (unsigned int i = 0; i < 5; i++)
    {
        if (clients[i].socket_fd == client_fd)
        {
            clients[i].socket_fd = 0;
            return ;
        }
    }
}

static bool read_client_data(int fd)
{
    char    buff[2048];

    int read_size;
    struct client* client = get_client(fd);
    read_size = read(fd, &buff, 2048);
    s_debug("Readed %d data on %d\n", read_size, fd);
    if (read_size <= 0)
        return false;
    client->readed_size = read_size;
    memcpy(client->readed_data, &buff, read_size);
    preprocess_data(client);
    return true;
}

/* Let's handle partial read in this */
void preprocess_data(struct client *client)
{
    unsigned int read_pos = 0;
    //printf("Pre:Read size : %d\n", client->readed_size);
    while (read_pos == 0 || read_pos < client->readed_size)
    {
        if (client->in_cmd)
        {
            //s_debug("Pre:WRITE");
            unsigned int to_complete_size = 0;
            if (client->current_cmd == WRITE_MEM)
                to_complete_size = client->write_info.size - client->write_buffer_size;
            if (client->current_cmd == PUT_FILE)
                to_complete_size = client->uploaded_file_size - client->uploaded_size;
            // Read data contains etheir all data or partial.
            if (to_complete_size <= client->readed_size - read_pos)
            {
                ///printf("get all write data\n");
                memcpy(client->pending_data, client->readed_data + read_pos, to_complete_size);
                client->pending_size = to_complete_size;
                read_pos += to_complete_size;
            } else {
                //printf("Partial write data\n");
                memcpy(client->pending_data, client->readed_data + read_pos, client->readed_size - read_pos);
                client->pending_size = client->readed_size - read_pos;
                read_pos = client->readed_size;
            }
            process_command(client);
            client->pending_pos = 0;
            client->pending_size = 0;
            continue ;
        }
        //s_debug("Pre:Checking \\n\n");
        bool has_endl = false;
        for (unsigned int i = read_pos; i < client->readed_size; i++)
        {
            if (client->readed_data[i] == '\n')
            {
                has_endl = true;
                memcpy(client->pending_data + client->pending_pos, client->readed_data + read_pos, i + 1 - read_pos);
                client->pending_size += i + 1 - read_pos;
                read_pos = i + 1;
                process_command(client);
                client->pending_pos = 0;
                client->pending_size = 0;
                //printf("read pos: %d\n", read_pos);
                break;
            }
        }
        if (has_endl)
            continue;
        // we should have reached unprocessed data
        memcpy(client->pending_data + client->pending_pos, client->readed_data + read_pos, client->readed_size - read_pos);
        client->pending_pos += client->readed_size - read_pos;
        client->pending_size += client->readed_size - read_pos;
        //printf("ppos, ppsize : %d, %d\n", client->pending_pos, client->pending_size);
        break;
    }
}

static  struct client* is_streaming_fd(int fd)
{
    for (unsigned int i = 0; i < 5; i++)
    {
        if (clients[i].socket_fd && clients[i].file_to_stream == fd)
            return &clients[i];
    }
    return NULL;
}

fd_set active_set;

void    add_streaming_fd(int new_fd)
{
    FD_SET(new_fd, &active_set);
}

int start_server()
{
    int server_socket;
    struct sockaddr_in name;

    for (unsigned int i = 0; i < 5; i++)
        clients[i].socket_fd = 0;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        fprintf(stderr, "Error while creating socket : %s\n", strerror(errno));
        return 1;
    }
    name.sin_family = AF_INET;
    name.sin_port = htons(1042);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_socket, (struct sockaddr*) &name, sizeof(name)) < 0)
    {
        fprintf(stderr, "Error while binding socket : %s\n", strerror(errno));
        return 1;
    }
    if (listen(server_socket, 1) < 0)
    {
        fprintf(stderr, "Error while listenning : %s\n", strerror(errno));
        return 1;
    }
    FD_ZERO(&active_set);
    FD_SET(server_socket, &active_set);
    s_debug("Server now running fd number is : %d\n", server_socket);
    while (true)
    {
        fd_set read_fd_set = active_set;
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        {
            fprintf(stderr, "Select error : %s\n", strerror(errno));
            return 1;
        }
        s_debug("==Something ready==\n");
        for (unsigned int i = server_socket; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &read_fd_set))
            {
                if (i == server_socket)
                {
                    s_debug("New client connection\n");
                    int new_socket;
                    int size = sizeof(struct sockaddr_in);
                    struct sockaddr_in new_client;
                    new_socket = accept(server_socket, (struct sockaddr *) &new_client, &size);
                    if (new_socket < 0)
                    {
                        fprintf(stderr, "Error while listenning : %s\n", strerror(errno));
                        return 1;
                    }
                    if (add_to_clients(new_socket))
                    {
                        s_debug ("Server: connection from host %s, port %hd.\n",
                            inet_ntoa (new_client.sin_addr),
                            ntohs (new_client.sin_port));
                        FD_SET(new_socket, &active_set);
                    } else {
                        fprintf(stderr, "execing max client number : %d", MAX_CLIENT);
                        close(new_socket);
                    }
                
                } else {
                    //s_debug("FD %d is ready\n", i);
                    struct client* streaming_client = is_streaming_fd(i);
                    if (streaming_client != NULL)
                    {
                        s_debug("A streamed file is ready for read\n");
                        char m_buffer[512];
                        int  readed;
                        while ((readed = read(i, &m_buffer, 512)) > 0)
                        {
                            s_debug("Readed %d on the streamed file", readed);
                            write(streaming_client->socket_fd, m_buffer, readed);
                        }
                        continue;
                    }
                    if (read_client_data(i) == false)
                    {
                        s_debug("Disconnecting client\n");
                        struct client* to_del_client = get_client(i);
                        if (to_del_client->file_to_stream)
                        {
                            FD_CLR(to_del_client->file_to_stream, &active_set);
                            close(to_del_client->file_to_stream);
                        }
                        remove_client(i);
                        FD_CLR(i, &active_set);
                        close(i);
                        s_debug("Piko\n");
                    }
                }
            }
        }
        s_debug("==End check\n");
    }
}