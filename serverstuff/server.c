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
    memcpy(client->pending_data + client->pending_size, &buff, read_size);
    client->pending_size += read_size;
    process_command(client);
    return true;
}

int main(int ac, char *ag[])
{
    int server_socket;
    struct sockaddr_in name;
    fd_set active_set;

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
        FD_SET(server_socket, &active_set);
        s_debug("SELECT - %d\n", FD_ISSET(server_socket, &active_set));
        if (select(FD_SETSIZE, &active_set, NULL, NULL, NULL) < 0)
        {
            fprintf(stderr, "Select error : %s\n", strerror(errno));
            return 1;
        }
        for (unsigned int i = 0; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &active_set))
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
                    s_debug("FD %d is ready\n", i);
                    if (read_client_data(i) == false)
                    {
                        s_debug("Disconnecting client\n");
                        remove_client(i);
                        FD_CLR(i, &active_set);
                        close(i);
                        s_debug("Piko\n");
                    }
                }
            }
        }
    }
}