#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>

int createTCPServer()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        perror("Unable to create TCP Socket\n");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);

    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("TCP Socket bind failure\n");
        return -1;
    }

    if(listen(sockfd, 5) == -1)
    {
        perror("Unable to listen at created TCP socket\n");
        return -1;
    }

    printf("TCP server listening at port %d\n", ntohs(addr.sin_port));

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
    if(client_sockfd == -1)
    {
        perror("Unable to accept the client's connection\n");
        return -1;
    }    

    char buffer[1024];
    int bytes_received = recv(client_sockfd, buffer, sizeof(buffer), 0);
    if(bytes_received == -1)
    {
        perror("Received error\n");
        return -1;
    }

    buffer[bytes_received] = '\0';
    printf("%s\n", buffer);

    close(client_sockfd);
    close(sockfd);

    return 0;
}


int main(void)
{
    createTCPServer();
    
    return 0;
}