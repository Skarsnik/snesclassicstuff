#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

void    my_send(int socket, char* buf)
{
    send(socket, buf, strlen(buf), 0);
}

int main(int ac, char* ag[])
{
    int sock;
    struct hostent* hostinfo = NULL;
    struct sockaddr_in sin;
    char                buf[2048];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    sin.sin_port = htons(1042);
    sin.sin_family = AF_INET;
    if (connect(sock, (struct sockaddr*) &sin, sizeof(sin)) < 0)
    {
        fprintf(stderr, "Error connecting : %s\n", strerror(errno));
        return 1;
    }
    my_send(sock, "CMD ls\n");
    int rsize;
    rsize = recv(sock, &buf, 2048, 0);
    printf("Received : %d bytes\n", rsize);
    write(1, &buf, rsize);
    my_send(sock, "WRITE_MEM 42 42 2\n");
    memset(&buf, 0, 5);
    send(sock, &buf, 2, 0);
    recv(sock, &buf, 2048, 0);
    printf("Write returned : %s", buf);
}