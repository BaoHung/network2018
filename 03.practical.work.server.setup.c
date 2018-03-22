#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int sockfd, clen, clientfd;
    struct sockaddr_in saddr, caddr;
    unsigned short port = 8784;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Error creating socket\n");
    }
    else
    {
        printf("Socket created\n");
    }
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        printf("Error binding\n");
    }
    else
    {
        printf("Binding success\n");
    }
    if (listen(sockfd, 5) < 0)
    {
        printf("Error listening\n");
    }
    else
    {
        printf("I'm listening...\n");
    }
    clen = sizeof(caddr);

    int countConnection = 0;
    while (countConnection++ < 4)
    {
        if ((clientfd = accept(sockfd, (struct sockaddr *)&caddr, &clen)) < 0)
        {
            printf("Error accepting connection\n");
        }
        else
        {
            printf("Connection accepted... %d\n", countConnection);
            close(clientfd);
        }
    }
    return 0;
}