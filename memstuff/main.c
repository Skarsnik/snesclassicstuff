#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define _GNU_SOURCE
#define __USE_GNU
#include <sys/uio.h>
#include <errno.h>

#ifdef MY_DEBUG
#define s_debug(...)  printf(__VA_ARGS__)
#else
#define s_debug(...)
#endif

void usage()
{
    printf("memstuff pid read address length\n");
    printf("\tRead the memory of the pid at the addresse given\n");
    printf("memstuff pid write address length stufftowrite\n");
    printf("\tWrite the memory of the pid with the data given\n");
    printf("memstuff pid search startaddress length data\n");
    printf("\tSearch in the pid memory the data and return the address\n");
}


char* hexstr_to_cstr(char *hexstr)
{
    char* c_str = (char*) malloc(strlen(hexstr) / 2 + 1);
    unsigned int hl = strlen(hexstr);
    s_debug("%s : %d\n", hexstr, hl);
    for (unsigned int i = 0; i < hl; i += 2)
    {
        sscanf(hexstr + i, "%2hhX", &c_str[i / 2]);
    }
    c_str[hl / 2] = 0;
    return c_str;
}

int    read_memory(pid_t pid, void* start, unsigned int length, void *out)
{
    struct iovec read_from[1];
    struct iovec read_into[1];
    read_from[0].iov_base = start;
    read_from[0].iov_len = length;
    read_into[0].iov_base = out;
    read_into[0].iov_len = length;
    s_debug("Address to read : %d - %x\n", start, start);
    ssize_t read_ret = process_vm_readv(pid, read_into, 1, read_from, 1, 0);
    s_debug("lenght read: %d\n", read_ret);
    if (read_ret < 0)
    {
        fprintf(stderr, "Error reading the process memory: %s\n", strerror(errno));
        return -1;
    }
    return read_ret;
}


int main(int argc, char **argv) {
    pid_t pid;
    char* cmd;
    char* addr_str;
    
    if (argc < 2)
    {
        usage();
        return 1;
    }
    pid = atoi(argv[1]);
    cmd = argv[2];

    if (strcmp(cmd, "read") == 0)
    {
        ssize_t addr;
        unsigned int length; 
        if (argc != 5)
        {
            usage();
            return 1;
        }
        addr_str = argv[3];
        sscanf(addr_str, "%zx", &addr);
        sscanf(argv[4], "%d", &length);
        void*   dest = malloc(length);
        int     readed_memory = read_memory(pid, (void*) addr, length, dest);
        if (readed_memory == -1)
            return 1;
        char *content = (char*) dest;
        for (unsigned int i = 0; i < readed_memory; i++)
        {
            printf("%02X", content[i]);
        }
        printf("\n");
        /*for (unsigned int i = 0; i < readed_memory; i++)
        {
            printf("%c", content[i]);
        }
        printf("\n");*/
        return 0;
    }
    if (strcmp(cmd, "search") == 0)
    {
        ssize_t addr;
        unsigned int length;
        if (argc != 6)
        {
            usage();
            return 1;
        }
        addr_str = argv[3];
        sscanf(addr_str, "%zx", &addr);
        sscanf(argv[4], "%d", &length);
        void* dest = malloc(length);
        if (read_memory(pid, (void*) addr, length, dest) == -1)
            return 1;
        char* content = (char*) dest;
        char *tofind = hexstr_to_cstr(argv[5]);
        printf("Trying to find : %s\n", tofind);
        unsigned int tofind_length = strlen(tofind);
        for (unsigned int i = 0; i < length; i++)
        {
           if (memcmp(content + i, tofind, tofind_length) == 0)
           {
               printf("%X\n", addr + i);
               return 0;
           }
        }
        fprintf(stderr, "Data not found\n");
        return 1;
    }
    if (strcmp(cmd, "write") == 0)
    {
        ssize_t addr;
        unsigned int length;
        if (argc != 6)
        {
            usage();
            return 1;
        }
        addr_str = argv[3];
        sscanf(addr_str, "%zx", &addr);
        sscanf(argv[4], "%d", &length);
        char *towrite = hexstr_to_cstr(argv[5]);
        if (length != strlen(argv[5]) / 2)
        {
            fprintf(stderr, "Write error: lenght specified and lenght of data does not match.\n");
            return 1;
        }
        struct iovec write_from[1];
        struct iovec write_into[1];
        write_from[0].iov_base = (void*) towrite;
        write_from[0].iov_len = length;
        write_into[0].iov_base = (void*) addr;
        write_into[0].iov_len = length;
        ssize_t write_ret = process_vm_writev(pid, write_from, 1, write_into, 1, 0);
        s_debug("lenght written: %d\n", write_ret);
        if (write_ret < 0)
        {
            fprintf(stderr, "Error writing the process memory: %s\n", strerror(errno));
            return -1;
        }
    }
    
    return 0;
}
