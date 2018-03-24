#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

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

#pragma region Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Error creating socket\n");
    }
    else
    {
        printf("Socket created\n");
    }
#pragma endregion

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

            // Reusing address
            setsockopt(sockfd, SOL_SOCKET,
                       SO_REUSEADDR, &(int){1},
                       sizeof(int));

#pragma region Enable nonblocking option
            int fl = fcntl(sockfd, F_GETFL, 0);
            fl |= O_NONBLOCK;
            fcntl(sockfd, F_SETFL, fl);
            printf("Enable nonblocking option\n");
#pragma endregion

            char msg[50];
            while (1)
            {
                printf("Client: ");
                scanf("%s", msg);
                send(sockfd, msg, strlen(msg), 0);
                memset(&msg, 0, sizeof(msg));
                if (recv(sockfd, msg, sizeof(msg), 0) > 0)
                {
                    printf("\nServer said: %s\n", msg);
                }
            }
        }
    }
    return 0;
}