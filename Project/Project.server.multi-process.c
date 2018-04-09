#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENT 100

int parentWritePipes[MAX_CLIENT][2], parentReadPipes[MAX_CLIENT][2], childPipeIndex, childClientFD;

void *stdInputChild()
{
    printf("Create thread in child\n");
    char msgFromParent[200];
    memset(msgFromParent, 0, sizeof(msgFromParent));
    close(parentWritePipes[childPipeIndex][1]);

    if (childClientFD > 0)
        while (read(parentWritePipes[childPipeIndex][0], &msgFromParent, sizeof(msgFromParent)) > 0)
        {
            // printf("Got_message_from_parent:_%s\n", msgFromParent);
            // snprintf(msgToSend, sizeof(msgToSend), "%s", msgFromParent);
            send(childClientFD, msgFromParent, strlen(msgFromParent), 0);
            memset(msgFromParent, 0, sizeof(msgFromParent));
        }
    close(parentWritePipes[childPipeIndex][0]);
    return 0;
}
void *stdInputParent(void *index)
{
    int i = *((int *)index);
    char msgFromChild[200];
    memset(msgFromChild, 0, sizeof(msgFromChild));
    close(parentReadPipes[i][1]);
    printf("Create thread in parent: %d\n", i);
    while (read(parentReadPipes[i][0], &msgFromChild, sizeof(msgFromChild)) > 0)
    {
        // Send message to all users
        // Not work in child process because fork will copy clientfds and not update
        for (int j = 0; j < MAX_CLIENT; j++)
        {
            if (parentWritePipes[j][1] > 0 && i != j) // only write to other pipes
            {
                close(parentWritePipes[j][0]);
                // printf("writing to pipe %d: %s\n", j, msgFromChild);
                // printf("WRITE %li\n", write(parentWritePipes[j][1], msgFromChild, strlen(msgFromChild)));
                write(parentWritePipes[j][1], msgFromChild, strlen(msgFromChild));
                // close(parentWritePipes[j][1]);
            }
        }
        memset(msgFromChild, 0, sizeof(msgFromChild));
    }
    return 0;
}

int main(int argc, char **argv)
{
    int sockfd, clientfd, clientfds[MAX_CLIENT], pid;
    unsigned int clen;
    struct sockaddr_in saddr, caddr;
    unsigned short port = 8784;
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
        // Non blocking
        int fl = fcntl(sockfd, F_GETFL, 0);
        fl |= O_NONBLOCK;
        fcntl(sockfd, F_SETFL, fl);

        fd_set set;
        // declaration of the set
        FD_ZERO(&set);
        // clear the set
        FD_SET(sockfd, &set);
        // add listening sockfd to set
        int maxfd = sockfd;
        // a required value to pass to select()
        for (int i = 0; i < MAX_CLIENT; i++)
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
#pragma region Handle new client
            clientfd = accept(sockfd, (struct sockaddr *)&caddr, &clen);
            int i = 0;
            if (clientfd >= 0)
            {
                // make it nonblocking
                fl = fcntl(clientfd, F_GETFL, 0);
                fl |= O_NONBLOCK;
                fcntl(clientfd, F_SETFL, fl);
                // add it to the clientfds array
                for (i = 0; i < MAX_CLIENT; i++)
                {
                    if (clientfds[i] == 0)
                    {
                        clientfds[i] = clientfd;
                        printf("Client %d connected %s:%d\n", clientfd, inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));

                        // Create child process for each client
                        childPipeIndex = i;
                        childClientFD = clientfd;
                        if (pipe(parentWritePipes[i]) == -1)
                        {
                            perror("pipe parentWritePipes error");
                            exit(1);
                        }

                        if (pipe(parentReadPipes[i]) == -1)
                        {
                            perror("pipe parentReadPipes error");
                            exit(1);
                        }

                        pid = fork();

#pragma region FORK
                        switch (pid)
                        {
                        case -1:
                            perror("Fork Error!!!");
                            break;
                        case 0:
                            printf("CHILD\n");
                            close(sockfd);
                            close(parentReadPipes[i][0]);

                            char msgReceived[200], msgToSend[200];
                            memset(msgReceived, 0, sizeof(msgReceived));
                            memset(msgToSend, 0, sizeof(msgToSend));

                            if (clientfd > 0)
                            {
                                pthread_t inputThread;
                                pthread_create(&inputThread, NULL, stdInputChild, &clientfd);

                                int clientID = i + 1;

#pragma region Send notice of new client to other clients
                                snprintf(msgToSend, sizeof(msgToSend), "====Client %d has connected.====", clientID);
                                write(parentReadPipes[i][1], msgToSend, strlen(msgToSend));
                                memset(msgToSend, 0, sizeof(msgToSend));
#pragma endregion

                                while (1)
                                {
                                    if (recv(clientfd, msgReceived, sizeof(msgReceived), 0) > 0)
                                    {
                                        if (strcmp(msgReceived, ":quit") == 0)
                                        {
                                            // printf("Client %d has disconnected.\n", i);
                                            snprintf(msgToSend, sizeof(msgToSend), "====Client %d has disconnected.====", clientID);
                                            printf("%s", msgToSend);
                                            write(parentReadPipes[i][1], msgToSend, strlen(msgToSend));
                                            memset(msgToSend, 0, sizeof(msgToSend));

                                            close(parentReadPipes[i][1]);
                                            exit(0);
                                        }
                                        else
                                        {
                                            snprintf(msgToSend, sizeof(msgToSend), "Client %d: %s", clientID, msgReceived);
                                            write(parentReadPipes[i][1], msgToSend, strlen(msgToSend));
                                        }
                                    }
                                    memset(msgReceived, 0, sizeof(msgReceived));
                                    memset(msgToSend, 0, sizeof(msgToSend));
                                }
                            }
                            close(parentReadPipes[i][1]);
                            exit(0);
                            break;
                        default:
                            printf("PARENT\n");
                            pthread_t inputThread;
                            pthread_create(&inputThread, NULL, stdInputParent, &i);
                        }

#pragma endregion

                        break;
                    }
                }
            }
#pragma endregion
        }
    }
#pragma endregion
    return 0;
}