#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<syslog.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<signal.h>

int sockfd;
int client_sockfd;
FILE *file;

void signalInterruptHandler(int signo);

void signalInterruptHandler(int signo)
{
    if((signo == SIGTERM) || (signo = SIGINT))
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
        close(sockfd);
        printf("Gracefully handling SIGTERM\n");
        syslog(LOG_INFO,  "Caught signal, exiting");
        fclose(file);
        closelog();
        exit(EXIT_SUCCESS);
    }
}

int createTCPServer()
{
    char *data;

    signal(SIGINT, signalInterruptHandler);
    signal(SIGTERM, signalInterruptHandler);

    const char *filepath = "/var/tmp/aesdsocketdata";

    file = fopen(filepath, "a");
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
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);

    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        syslog(LOG_ERR, "TCP Socket bind failure");
        perror("TCP Socket bind failure\n");
        return -1;
    }

    if(listen(sockfd, 5) == -1)
    {
        syslog(LOG_ERR, "Unable to listen at created TCP socket");
        perror("Unable to listen at created TCP socket\n");
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
            return -1;
        }

        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        do
        {
            char buffer[1024];
            int bytes_received = recv(client_sockfd, buffer, sizeof(buffer), 0);
            if(bytes_received <= 0)
            {
                syslog(LOG_ERR, "Received error");
                perror("Received error\n");
                break;
            }


            data = strchr(buffer, '\n');

            buffer[bytes_received] = '\0';
            printf("%s\n", buffer);

            if(data != NULL)
            {
                data = '\0';
                fputs(buffer, file);
                fputs("\n", file);
                fflush(file);
                fseek(file, 0, SEEK_SET);
                syslog(LOG_INFO, "%s", data);
                printf("%s\n", data);

                while((bytes_received = fread(buffer, 1, sizeof(buffer), file)) > 0)
                {
                    send(client_sockfd, buffer, bytes_received, 0);
                }

                // break;
            }
        } while (1);
    
        close(client_sockfd);
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
    }

    return 0;
}


int main(void)
{
    createTCPServer();
    
    return 0;
}