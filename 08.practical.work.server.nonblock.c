#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

int main(int argc, char **argv)
{
    int sockfd, clen, clientfd;
    struct sockaddr_in saddr, caddr;
    unsigned short port = 8784;

#pragma region Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Error creating socket\n");
    }
    else
    {
        // Reusing address
        setsockopt(sockfd, SOL_SOCKET,
                   SO_REUSEADDR, &(int){1},
                   sizeof(int));
        printf("Socket created\n");
    }
#pragma endregion

#pragma region Enable nonblocking option
    int fl = fcntl(sockfd, F_GETFL, 0);
    fl |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, fl);
    printf("Enable nonblocking option\n");
#pragma endregion

#pragma region Binding
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        printf("Error binding\n");
        return -1;
    }
    else
    {
        printf("Binding success\n");
    }
#pragma endregion

#pragma region Listen on socket
    if (listen(sockfd, 5) < 0)
    {
        printf("Error listening\n");
    }
    else
    {
        printf("I'm listening...\n");
    }
    clen = sizeof(caddr);
#pragma endregion

#pragma region Accepting connection
    int waitCount = 0;
    while (1)
    {
        if ((clientfd = accept(sockfd, (struct sockaddr *)&caddr, &clen)) < 0)
        {
            printf("No connection, I'm still waiting... %d\n", waitCount++);
            sleep(1);
        }
        else
        {
            printf("Connection accepted!\n");

            // Non blocking client
            int fl = fcntl(clientfd, F_GETFL, 0);
            fl |= O_NONBLOCK;
            fcntl(clientfd, F_SETFL, fl);

            char msg[50];
            while (1)
            {
                if (recv(clientfd, msg, sizeof(msg), 0) > 0)
                {
                    printf("\nClient said: %s\n", msg);
                }
                printf("Server: ");
                scanf("%s", msg);
                send(clientfd, msg, strlen(msg), 0);
                memset(&msg, 0, sizeof(msg));
            }
            close(clientfd);
        }
    }
#pragma endregion
    return 0;
}
