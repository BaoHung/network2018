#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t message = PTHREAD_COND_INITIALIZER;
char msg[50];

void *stdInput()
{
    while (1)
    {
        pthread_mutex_lock(&mutex);
        scanf("%s", msg);
        pthread_cond_signal(&message);
        pthread_cond_wait(&message, &mutex);
        pthread_mutex_unlock(&mutex);
    }
}

void *network(void *sockfd)
{
    int fd = *((int *)sockfd);
    while (1)
    {
        pthread_mutex_lock(&mutex);
        if (strlen(msg) == 0)
        {
            pthread_cond_wait(&message, &mutex);
        }
        send(fd, msg, strlen(msg), 0);
        if (strcmp(msg, ":quit") == 0)
        {
            printf("Exiting program.\n");
            close(fd);
            exit(0);
        }
        memset(&msg, 0, sizeof(msg));
        pthread_cond_signal(&message);
        pthread_mutex_unlock(&mutex);
    }
}

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
            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

#pragma region Enable nonblocking option
            int fl = fcntl(sockfd, F_GETFL, 0);
            fl |= O_NONBLOCK;
            fcntl(sockfd, F_SETFL, fl);
            printf("Enable nonblocking option\n");
#pragma endregion

#pragma region Add threads for standard input and network
            void *status;
            pthread_t inputThread, networkThread;
            pthread_create(&inputThread, NULL, stdInput, NULL);
            pthread_create(&networkThread, NULL, network, &sockfd);
            printf("Say something: ");
            fflush(stdout);

            // Receiving and sending need to be in different threads
            char recvMsg[50];
            while (1)
            {
                while (recv(sockfd, recvMsg, sizeof(recvMsg), 0) > 0)
                {
                    printf("\n%s\n> ", recvMsg);
                    fflush(stdout);
                }
                memset(&recvMsg, 0, sizeof(recvMsg));
            }

            pthread_join(inputThread, &status);
            pthread_join(networkThread, &status);
#pragma endregion
        }
    }
    return 0;
}