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

int parentWritePipes[100][2], parentReadPipes[100][2], childPipeIndex, childClientFD;

void *stdInputChild(void *cfd)
{
    // int fd = *((int *)cfd);
    char msgFromParent[200];
    close(parentWritePipes[childPipeIndex][1]);

    if (childClientFD > 0)
        while (1)
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
void *stdInputParent(void *fds)
{
    char msgFromChild[200];
    int *clientfds = (int *)fds;
    while (1)
        while (scanf("%s", msgFromChild) != EOF) // read from child
        {

            // Send message to all users
            // Not work in child process because fork will copy clientfds and not update
            for (int j = 0; j < MAX_CLIENT; j++)
            {
                if (parentWritePipes[j][1] > 0)
                {
                    close(parentWritePipes[j][0]);
                    printf("writing to pipe %d: %s\n", j, msgFromChild);
                    write(parentWritePipes[j][1], msgFromChild, strlen(msgFromChild));
                    close(parentWritePipes[j][1]);
                }

                if (clientfds[j] > 0)
                {
                    // printf("%s\n", msgFromChild);
                    // fflush(stdout);
                    // printf("clientfds[%d]: %d\n", j, clientfds[j]);
                    // send(clientfds[j], msgFromChild, strlen(msgFromChild), 0);
                }
            }
            // printf("Through_to_parent===%s+++\n", msgFromChild);
        }
    return 0;
}

int main(int argc, char **argv)
{
    int sockfd, clientfd, clientfds[100], pid;
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
    // int waitCount = 0;
    while (1)
    {
        // Non blocking
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
            int i = 0;
            if (clientfd < 0)
            {
                printf("Accept failed\n");
                printf("clientfd: %d\n", clientfd);
                exit(1);
            }
            else
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
                        printf("Client_%d_connected_%s:%d\n", clientfd, inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));

                        // Create child process for each client
                        // pipe(parentWritePipes[i]);
                        // pipe(parentReadPipes[i]);

                        childPipeIndex = i;
                        childClientFD = clientfd;
                        if (pipe(parentWritePipes[i]) == -1)
                        {
                            perror("pipe parentWritePipes[i] error");
                            exit(1);
                        }

                        if (pipe(parentReadPipes[i]) == -1)
                        {
                            perror("pipe parentWritePipes[i] error");
                            exit(1);
                        }

                        pid = fork();
                        break;
                    }
                }
            }

#pragma region FORK
            switch (pid)
            {
            case -1:
                perror("Fork Error!!!");
                break;
            case 0:
                printf("CHILD\n");
                close(parentReadPipes[i][0]);
                dup2(parentReadPipes[i][1], 1);
                close(parentReadPipes[i][1]);

                char msgFromParent[200];
                // read(parentWritePipes[i][0], &msgFromParent, sizeof(msgFromParent));

                // printf("Pipe %d =======%s===============\n", i, msgFromParent);

                // is that data from a previously-connect client?
                char msgReceived[200], msgToSend[200];
                // printf("INIT---\n");

                memset(msgToSend, 0, sizeof(msgToSend));

                // if (!FD_ISSET(clientfd, &set))
                //     FD_SET(clientfd, &set);

                // if (clientfd > 0 && FD_ISSET(clientfd, &set))
                if (clientfd > 0)
                {
                    pthread_t inputThread;
                    pthread_create(&inputThread, NULL, stdInputChild, &clientfd);
                    while (1)
                    {
                        // printf("in_loop---\n");
                        // sleep(1);

                        if (recv(clientfd, msgReceived, sizeof(msgReceived), 0) > 0)
                        {
                            if (strcmp(msgReceived, ":quit") == 0)
                            {
                                printf("Client %d has disconnected.\n", clientfd);
                                // clientfds[i] = 0;
                                exit(0);
                            }
                            else
                            {
                                printf("Client_%d:_%s\n", clientfd, msgReceived);
                                fflush(stdout);
                                // printf("WRITE %li\n", write(parentReadPipes[i][1], msgReceived, sizeof(msgReceived)));
                            }
                        }
                        // else
                        // {
                        //     // some error. remove it from the "active" fd array
                        //     printf("Client %d has disconnected.\n", clientfd);
                        //     exit(0);
                        // }
                        memset(msgReceived, 0, sizeof(msgReceived));

                        // printf("Done Client.\n");

                        // printf("Waiting_for_parent.\n");
                        // if (scanf("%s", msgFromParent) != EOF)
                        // {
                        //     printf("Got_message_from_parent:_%s\n", msgFromParent);
                        //     snprintf(msgToSend, sizeof(msgToSend), "%s", msgFromParent);
                        //     send(clientfd, msgToSend, strlen(msgToSend), 0);
                        //     memset(msgToSend, 0, sizeof(msgToSend));
                        // }
                        // printf("Done_waiting_for_parent.\n");
                    }
                    printf("END loop.\n");
                }
                // close(parentWritePipes[i][1]);
                // close(parentWritePipes[i][0]);
                exit(0);
                break;
            default:
                // printf("PARENT\nChild's_pid_is:_%d\n", pid);
                // close(sockfd);
                // close(parentWritePipes[i][0]);
                close(parentReadPipes[i][1]);

                dup2(parentReadPipes[i][0], 0);
                // dup2(parentWritePipes[i][1], 1);

                // close(parentWritePipes[i][1]);
                close(parentReadPipes[i][0]);

                pthread_t inputThread;
                pthread_create(&inputThread, NULL, stdInputParent, clientfds);
                // time_t current_time;
                /* Convert to local time format. */
                // char c_time_string[200];
                // current_time = time(NULL);
                // snprintf(c_time_string, sizeof(c_time_string), "%s", ctime(&current_time));
                // write(parentWritePipes[i][1], c_time_string, strlen(c_time_string));
                // memset(c_time_string, 0, sizeof(c_time_string));

                char msgBuffer[200], msgFromChild[200];
                // snprintf(msgBuffer, sizeof(msgBuffer), "From server Parent\n");
                // write(parentWritePipes[i][1], msgBuffer, sizeof(msgBuffer));
                // memset(msgBuffer, 0, sizeof(msgBuffer));

                // Read from child
                // printf("Waiting for child.\n");

                // printf("Done scanf\n");

                // while (1)
                // {
                //     printf("dummy\n");
                //     sleep(1);
                // }

                // printf("Done reading\n");

                // while (1)
                // {
                // printf("In Parent, sending dummy\n");
                // snprintf(msgBuffer, sizeof(msgBuffer), "000\n");
                // write(parentWritePipes[i][1], msgBuffer, sizeof(msgBuffer));
                // memset(msgBuffer, 0, sizeof(msgBuffer));
                // }
                // break;
            }

#pragma endregion
        }
#pragma endregion
    }
#pragma endregion
    return 0;
}