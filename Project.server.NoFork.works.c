#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/select.h>

#define MAX_CLIENT 100

int main(int argc, char **argv)
{
    int sockfd, clientfd;
    unsigned int clen;
    struct sockaddr_in saddr, caddr;
    unsigned short port = 8784;
    int clientfds[100];
    memset(clientfds, 0, sizeof(clientfds));

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
    while (1)
    {
        // Non blocking client
        int fl = fcntl(sockfd, F_GETFL, 0);
        fl |= O_NONBLOCK;
        fcntl(sockfd, F_SETFL, fl);

#pragma region Multiplexing Handling
        fd_set set;
        // declaration of the set
        FD_ZERO(&set);
        // clear the set
        FD_SET(sockfd, &set);
        // add listening sockfd to set
        int maxfd = sockfd;
        // a required value to pass to select()
        for (int i = 0; i < 100; i++)
        {
            // add connected client sockets to set
            if (clientfds[i] > 0)
            {
                FD_SET(clientfds[i], &set);
            }
            if (clientfds[i] > maxfd)
            {
                maxfd = clientfds[i];
            }
        }
        // poll and wait, blocked indefinitely
        select(maxfd + 1, &set, NULL, NULL, NULL);

        // a «listening» socket?
        if (FD_ISSET(sockfd, &set))
        {
            clientfd = accept(sockfd, (struct sockaddr *)&caddr, &clen);
            // make it nonblocking
            fl = fcntl(clientfd, F_GETFL, 0);
            fl |= O_NONBLOCK;
            fcntl(clientfd, F_SETFL, fl);
            // add it to the clientfds array
            for (int i = 0; i < MAX_CLIENT; i++)
            {
                if (clientfds[i] == 0)
                {
                    clientfds[i] = clientfd;
                    break;
                }
            }
        }

        // is that data from a previously-connect client?
        int i;
        char msg[200], msgToSend[200];
        memset(msg, 0, sizeof(msg));
        for (i = 0; i < MAX_CLIENT; i++)
        {
            if (clientfds[i] > 0 && FD_ISSET(clientfds[i], &set))
            {
                if (read(clientfds[i], msg, sizeof(msg)) > 0)
                {
                    snprintf(msgToSend, sizeof(msgToSend), "Client %d: %s", clientfds[i], msg);
                    printf("%s\n", msgToSend);
                    // Send message to all users
                    for (int j = 0; j < MAX_CLIENT; j++)
                    {
                        if (clientfds[j] > 0 && clientfds[i] != clientfds[j]) // send to other clients
                        {
                            send(clientfds[j], msgToSend, strlen(msgToSend), 0);
                        }
                    }
                }
                else
                {
                    // some error. remove it from the "active" fd array
                    printf("Client %d has disconnected.\n", clientfds[i]);
                    clientfds[i] = 0;
                }
                memset(msg, 0, sizeof(msg));
            }
        }
#pragma endregion
    }
#pragma endregion
    return 0;
}
