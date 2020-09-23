#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "server.h"


int write_size_expected = 4;

void    print_data(const char* data, const unsigned int size)
{
    for (unsigned int i = 0; i < size; i++)
    {
        if (isalnum(data[i]) || data[i] == ' ')
            printf("%c", data[i]);
        else
            printf("$%02X", (unsigned char) data[i]);
    }
    printf("\n");
}

void    process_command(struct client* client)
{
    static int write_size = 0;

    printf("Processing command : %d\n", client->pending_size);
    print_data(client->pending_data, client->pending_size);
    if (client->in_cmd == true)
    {
        printf("Data are write data\n");
        write_size += client->pending_size;
        if (client->write_info.size == write_size)
        {
            write_size = 0;
            client->in_cmd = false;
        }
        return ;
    }
    if (strncmp(client->pending_data, "WRITE_MEM\n", 10) == 0)
    {
        client->write_info.size = write_size_expected;
        client->write_buffer_size = 0;
        client->in_cmd = true;
        client->current_cmd = WRITE_MEM;
    }
}

static void write_to_client(struct client* client, const char* to_write)
{
    memcpy(client->readed_data, to_write, strlen(to_write));
    client->readed_size = strlen(to_write);
}

int main(int ac, char *ag[])
{
    struct client client;

    client.pending_size = 0;
    client.pending_pos = 0;
    client.in_cmd = false;

    /*printf("==Simple command\n");
    write_to_client(&client, "CMD ls\n");
    preprocess_data(&client);
    printf("==Two commands\n");
    write_to_client(&client, "CMD ls\nCMD ls2\n");
    preprocess_data(&client);
    printf("==Partial cmd write 2 parts \n");
    write_to_client(&client, "CMD");
    preprocess_data(&client);
    write_to_client(&client, " ls\n");
    preprocess_data(&client);
    printf("==Partial cmd write 3 parts\n");
    write_to_client(&client, "CMD");
    preprocess_data(&client);
    write_to_client(&client, " ls");
    preprocess_data(&client);
    write_to_client(&client, "\n");
    preprocess_data(&client);*/

    printf("==\n");
    write_to_client(&client, "CMD pidof canoe-shvc\n");
    preprocess_data(&client);
    write_to_client(&client, "CMD ps | grep canoe-shvc | grep -v grep\n");
    preprocess_data(&client);
    write_to_client(&client, "CMD pmap 2817 -x -q | grep -v canoe-shvc | grep -v /lib | grep rwx | grep anon\n");
    preprocess_data(&client);
    write_to_client(&client, "READ_MEM 2817 b5fecb86 6\n");
    preprocess_data(&client);

return 0;

    printf("==Write mem normal usage\n");
    write_to_client(&client, "WRITE_MEM\n");
    preprocess_data(&client);
    write_to_client(&client, "PIKO");
    preprocess_data(&client);

    printf("==Write mem + data\n");
    write_to_client(&client, "WRITE_MEM\nPIKO");
    preprocess_data(&client);
    write_size_expected = 1;
    printf("==Write mem + data 1 size\n");
    write_to_client(&client, "WRITE_MEM\nP");
    preprocess_data(&client);

    write_size_expected = 4;
    printf("==Write mem cut\n");
    write_to_client(&client, "WRITE_M");
    preprocess_data(&client);
    write_to_client(&client, "EM\nPIKO");
    preprocess_data(&client);
    printf("==Write mem 3 part\n");
    write_to_client(&client, "WRITE_M");
    preprocess_data(&client);
    write_to_client(&client, "EM\n");
    preprocess_data(&client);
    write_to_client(&client, "PIKO");
    preprocess_data(&client);
    printf("==Write mem 4 part\n");
    write_to_client(&client, "WRITE_M");
    preprocess_data(&client);
    write_to_client(&client, "EM\n");
    preprocess_data(&client);
    write_to_client(&client, "PI");
    preprocess_data(&client);
    write_to_client(&client, "KO");
    preprocess_data(&client);
    
    printf("==Mix\n");
    write_to_client(&client, "WRITE_MEM\nPIKOCMD ls\n");
    preprocess_data(&client);
}