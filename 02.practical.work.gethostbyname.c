#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>

void getIP(char *hostname)
{
    struct hostent *ht = gethostbyname(hostname);
    if (ht != NULL)
    {
        int i = 0;
        printf("IP addresses for %s: \n", hostname);
        while (ht->h_addr_list[i] != NULL)
            printf("%s\n", inet_ntoa(*(struct in_addr *)ht->h_addr_list[i++]));
        printf("------------------\n");
    }
    else
    {
        printf("Cannot get host name.\n");
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        char hostname[256];
        printf("Enter host name: ");
        scanf("%s", hostname);
        getIP(hostname);
    }
    else
    {
        int i;
        for (i = 1; i < argc; i++)
        {
            getIP(argv[i]);
        }
    }

    return 0;
}