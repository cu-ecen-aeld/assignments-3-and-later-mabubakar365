#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<syslog.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<signal.h>
#include<stdbool.h>

int sockfd;
int client_sockfd;
FILE *file;

void signalInterruptHandler(int signo);

void signalInterruptHandler(int signo)
{
    if((signo == SIGTERM) || (signo == SIGINT))
    {
        int Status;

        Status = remove("/var/tmp/aesdsocketdata");
        if(Status == 0)
        {
            printf("Sucesfully deleted file /var/tmp/aesdsocket\n");
        }

        else
        {
            printf("Unable to delete file at path /var/tmp/aesdsocket\n");
        }

        close(client_sockfd);
        printf("Gracefully handling SIGTERM\n");
        syslog(LOG_INFO,  "Caught signal, exiting");
        close(sockfd);
        fclose(file);
        closelog();
        exit(EXIT_SUCCESS);
    }
}

int createTCPServer()
{
    char *buffer = NULL;

    signal(SIGINT, signalInterruptHandler);
    signal(SIGTERM, signalInterruptHandler);

    const char *filepath = "/var/tmp/aesdsocketdata";
    ssize_t num_bytes; 

    file = fopen(filepath, "a+");
    if(file == NULL)
    {
        perror("Unable to open or create the file");
        return -1;
    }

    openlog("aesdsocket.c", LOG_CONS | LOG_PID, LOG_USER);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        syslog(LOG_ERR, "Unable to create TCP Socket");
        perror("Unable to create TCP Socket\n");
        fclose(file);
        closelog();
        return -1;
    }

    int enable = 1;
    if(setsockopt(sockfd, SOL_SOCKET,SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        syslog(LOG_ERR, "setsockopt(SO_REUSEADDR) failed");
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);

    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        syslog(LOG_ERR, "TCP Socket bind failure");
        perror("TCP Socket bind failure\n");
        close(sockfd);
        fclose(file);
        closelog();        
        return -1;
    }

    if(listen(sockfd, 5) == -1)
    {
        syslog(LOG_ERR, "Unable to listen at created TCP socket");
        perror("Unable to listen at created TCP socket\n");
        close(sockfd);
        fclose(file);
        closelog();        
        return -1;
    }

    syslog(LOG_INFO, "TCP server listening at port %d", ntohs(addr.sin_port));
    printf("TCP server listening at port %d\n", ntohs(addr.sin_port));

    while(1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if(client_sockfd == -1)
        {
            syslog(LOG_ERR, "Unable to accept the client's connection");
            perror("Unable to accept the client's connection\n");
            close(sockfd);
            fclose(file);
            closelog();            
            return -1;
        }  

        char client_ip[INET6_ADDRSTRLEN];

        if(getpeername(client_sockfd, (struct sockaddr *)&client_addr, &client_len) == 0)
        {
            if(client_addr.sin_family == AF_INET)
            {
                struct sockaddr_in *ipv4 = (struct sockaddr_in *)&client_addr;
                inet_ntop(AF_INET, &(ipv4->sin_addr), client_ip, INET6_ADDRSTRLEN);
            }

            else if(client_addr.sin_family == AF_INET6)
            {
                struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&client_addr;
                inet_ntop(AF_INET6, &(ipv6->sin6_addr), client_ip, INET6_ADDRSTRLEN);
            }
            
            printf("Client IP Address: %s\n", client_ip);
        }

        else
        {
            perror("Unable to get client IP address");
            close(sockfd);
            fclose(file);
            closelog();
            return -1;
        }

        printf("Accepted connection from %s\n", client_ip);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        buffer = NULL;
        num_bytes = 0;

        while(1)
        {
            int connection_closed = 0;
            int received_error = 0;

            do {
                buffer = realloc(buffer, num_bytes + 1024);
                if (!buffer) {
                    syslog(LOG_INFO, "Unable to allocate space on heap");
                    printf("Unable to allocate space on heap\n");
                    close(sockfd);
                    fclose(file);
                    closelog();
                    return -1;
                }

                ssize_t recv_bytes = recv(client_sockfd, buffer + num_bytes, 1024, 0);

                if(recv_bytes == 0)
                {
                    syslog(LOG_INFO, "Closed connection from %s", client_ip);
                    printf("Closed connection from %s\n", client_ip);
                    connection_closed = 1;
                    break;
                }

                else if(recv_bytes < 0)
                {
                    syslog(LOG_ERR, "Received error");
                    perror("Received error\n");
                    received_error = 1;
                    break;
                }

                num_bytes += recv_bytes;
            } while (!strchr(buffer, '\n'));   

            if(received_error == 1 || connection_closed == 1) 
            {
                free(buffer);
                buffer = NULL;
                num_bytes = 0;  // Reset num_bytes to 0
                break;
            }

            else
            {
                buffer[num_bytes] = '\0';

                printf("Packet Received %s\n", buffer);

                fputs(buffer, file);
                fflush(file);
                fseek(file, 0, SEEK_SET);

                size_t bufferSize = 1024; // or any other size you want
                char* writeBuf = malloc(bufferSize * sizeof(char));
                if(writeBuf == NULL) {
                    perror("Unable to allocate memory for writeBuf");
                    return -1;
                }

                while(fgets(writeBuf, bufferSize, file) != NULL)
                {
                    send(client_sockfd, writeBuf, strlen(writeBuf), 0);
                }

                free(writeBuf); // don't forget to free the memory when you're done with it
                                

                free(buffer);
                buffer = NULL;
                num_bytes = 0;  // Reset num_bytes to 0
            }

        }

        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        if (remove(filepath) == 0) printf("Deleted successfully\n");
        else printf("Unable to delete the file\n");
        close(client_sockfd);            
    }
    close(sockfd);
    fclose(file);
    closelog();               
    return 0; 
}

int main(void)
{
    if(createTCPServer() == -1) printf("Error in running application\n");
    
    return 0;
}