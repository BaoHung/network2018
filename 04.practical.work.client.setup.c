#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char **argv)
{
    struct sockaddr_in saddr;
    struct hostent *h;
    int sockfd;
    unsigned short port = 8784;
    char hostname[256];

    if (argc < 2)
    {
        printf("Enter host name: ");
        scanf("%s", hostname);
    }
    else
    {
        strcpy(hostname, argv[1]);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Error creating socket\n");
    }
    else
    {
        printf("Socket created\n");
    }
    
    if ((h = gethostbyname(hostname)) == NULL)
    {
        printf("Unknown host\n");
    }
    else
    {
        printf("IP address: %s\n", inet_ntoa(*(struct in_addr *)h->h_addr_list[0]));
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        memcpy((char *)&saddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
        saddr.sin_port = htons(port);
        if (connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
        {
            printf("Cannot connect\n");
        }
        else
        {
            printf("Connected\nYAYAY!!!\n");
        }
    }
    return 0;
}