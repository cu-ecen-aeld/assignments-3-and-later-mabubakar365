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
#include<sys/stat.h>
#include<pthread.h>
#include<time.h>
#include<sys/time.h>
#include "../aesd-char-driver/aesd_ioctl.h"


#define USE_AESD_CHAR_DEVICE 1

#ifdef USE_AESD_CHAR_DEVICE
#include<fcntl.h>
#endif

int sockfd;

#ifndef USE_AESD_CHAR_DEVICE //change it later
const char *filepath = "/var/tmp/aesdsocketdata";
#else
const char *filepath = "/dev/aesdchar";
#endif

#ifndef USE_AESD_CHAR_DEVICE // change it later
FILE *file;
#else
int file;
#endif

pthread_mutex_t file_mutex;

void *handle_client(void *ptr);
void signalInterruptHandler(int signo);
int createTCPServer(int deamonize);

typedef struct node {
    pthread_t tid;
    struct node *next;
} Node;

typedef struct {
    int client_sockfd;
    struct sockaddr_in client_addr;
} client_info_t;

void *handle_client(void *ptr)
{
    client_info_t *client_info = (client_info_t *)ptr;
    int client_sockfd = client_info->client_sockfd;
    struct sockaddr_in client_addr = client_info->client_addr;
    free(client_info);
    
    socklen_t client_len = sizeof(client_addr);

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
        
        // printf("Client IP Address: %s\n", client_ip);
    }

    else
    {
        perror("Unable to get client IP address");
        close(client_sockfd);
        return NULL;
    }

    // printf("Accepted connection from %s\n", client_ip);
    syslog(LOG_INFO, "Accepted connection from %s", client_ip);

    char *buffer = NULL;
    // ssize_t num_bytes = 0;
    ssize_t recv_bytes = 0;
    struct aesd_seekto seekto;
    const char *pattern = "AESDCHAR_IOCSEEKTO:";
    uint32_t X, Y;
    bool needToReopen = 0;

    // while(1)
    // {
        int connection_closed = 0;
        int received_error = 0;
        bool isNewLineFound = 0;

        buffer = malloc(1024 * sizeof(char));
        if (!buffer) {
            syslog(LOG_INFO, "Unable to allocate space on heap");
            printf("Unable to allocate space on heap\n");
            close(sockfd);
            return NULL;
        }

        memset(buffer, 0, 1024 * sizeof(char));
        file = open(filepath, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        // printf("File opened for write\n");

        while(!isNewLineFound)
        {
            recv_bytes = recv(client_sockfd, buffer, 1024, 0);

            if(recv_bytes == 0)
            {
                syslog(LOG_INFO, "Closed connection from %s", client_ip);
                // printf("Closed connection from %s\n", client_ip);
                connection_closed = 1;
            }
            else if(recv_bytes < 0)
            {
                syslog(LOG_ERR, "Received error");
                perror("Received error\n");
                received_error = 1;
            }

            
            else if(received_error == 1 || connection_closed == 1) 
            {
                free(buffer);
                buffer = NULL;
                needToReopen = 0;
                // printf("File closed due to receive error\n");
                close(file);
                break;
            }    

            else
            {
                // printf("Data Received\n");
                char * match = strstr(buffer, pattern);
                if(match)
                {
                    if(sscanf(match, "AESDCHAR_IOCSEEKTO:%u,%u", &X, &Y) == 2)
                    {
                        seekto.write_cmd = X;
                        seekto.write_cmd_offset = Y;
                        ioctl(file, AESDCHAR_IOCSEEKTO, &seekto);
                        // printf("IOCTL Issued\n");
                        needToReopen = 0;
                        break;
                    }
                }

                write(file, buffer, recv_bytes);
                if(memchr(buffer, '\n', 1024*sizeof(char)) != NULL)
                {
                    isNewLineFound = 1;
                    lseek(file, 0, SEEK_SET);
                    needToReopen = 1;
                    // printf("Data written\n");
                    close(file);
                    // printf("File closed after write\n");
                }
                else
                {
                    memset(buffer, 0, 1024*sizeof(char));
                }
            }
        }

        pthread_mutex_lock(&file_mutex);
        #ifndef USE_AESD_CHAR_DEVICE // change it later
        fputs(buffer, file);
        // fflush(file);
        fseek(file, 0, SEEK_SET);
        #else
        #endif

        size_t bufferSize = 1024; // or any other size you want
        char* writeBuf = malloc(bufferSize * sizeof(char));
        if(writeBuf == NULL) {
            // perror("Unable to allocate memory for writeBuf");
            pthread_mutex_unlock(&file_mutex);
            return NULL;
        }

        #ifndef USE_AESD_CHAR_DEVICE // change it later
        while(fgets(writeBuf, bufferSize, file) != NULL)
        {
            send(client_sockfd, writeBuf, strlen(writeBuf), 0);
        }
        #else
        ssize_t bytesRead;
        if(needToReopen) 
        {
            file = open(filepath, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            // printf("File opened for read\n");
        }
        while ((bytesRead = read(file, writeBuf, bufferSize)) > 0) {
            send(client_sockfd, writeBuf, bytesRead, 0);
            // printf("Data Read\n");
        }
        printf("File closed after read\n");
        close(file);
        #endif

        free(writeBuf); // don't forget to free the memory when you're done with it
        
        pthread_mutex_unlock(&file_mutex);

        free(buffer);
        buffer = NULL;
        // num_bytes = 0;  // Reset num_bytes to 0
    // }

    syslog(LOG_INFO, "Closed connection from %s", client_ip);
    close(client_sockfd);  
    return NULL;
}

void signalInterruptHandler(int signo)
{
    if((signo == SIGTERM) || (signo == SIGINT))
    {
        #ifndef USE_AESD_CHAR_DEVICE // change it later
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
        #endif

        printf("Gracefully handling SIGTERM\n");
        syslog(LOG_INFO,  "Caught signal, exiting");
        close(sockfd);
        #ifndef USE_AESD_CHAR_DEVICE //change it later
        fclose(file);
        #else
        close(file);
        #endif
        pthread_mutex_destroy(&file_mutex);
        closelog();
        exit(EXIT_SUCCESS);
    }

    if(signo == SIGALRM)
    {
        #ifndef USE_AESD_CHAR_DEVICE // change it later
        struct timeval tv;
        struct tm ptm;
        char time_string[40];
        long milliseconds;

        // Get the current time
        gettimeofday(&tv, NULL);

        // Format the date and time, down to a single second
        localtime_r(&tv.tv_sec, &ptm);
        strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", &ptm);

        // Compute milliseconds from microseconds
        milliseconds = tv.tv_usec;

        alarm(10);
        // Print the formatted time, in seconds, milliseconds and time zone offset
        printf("%s.%06ld %c%02d%02d\n", time_string, milliseconds, 
            (ptm.tm_gmtoff < 0) ? '-' : '+',
            abs(ptm.tm_gmtoff) / 3600,
            (abs(ptm.tm_gmtoff) % 3600) / 60);

        pthread_mutex_lock(&file_mutex);

        char buffer[1024];
        int len = snprintf(buffer, sizeof(buffer), "timestamp:%s.%06ld %c%02d%02d\n", time_string, milliseconds, 
            (ptm.tm_gmtoff < 0) ? '-' : '+',
            abs(ptm.tm_gmtoff) / 3600,
            (abs(ptm.tm_gmtoff) % 3600) / 60);

        file = open(filepath, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (write(file, buffer, len) != len) {
            // Handle error
            perror("write");
        }

        else 
        {
            lseek(file, 0, SEEK_SET);
            close(file);
        }
        // fprintf(file, "timestamp:%s.%06ld %c%02d%02d\n", time_string, milliseconds, 
        //     (ptm.tm_gmtoff < 0) ? '-' : '+',
        //     abs(ptm.tm_gmtoff) / 3600,
        //     (abs(ptm.tm_gmtoff) % 3600) / 60);
        // // fflush(file);   
        // fseek(file, 0, SEEK_SET);
        pthread_mutex_unlock(&file_mutex);
        #endif
    }
}

int createTCPServer(int deamonize)
{
    signal(SIGINT, signalInterruptHandler);
    signal(SIGTERM, signalInterruptHandler);

    #ifndef USE_AESD_CHAR_DEVICE // change it later
    file = fopen(filepath, "a+");
    #else
    file = open(filepath, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    #endif

    #ifndef USE_AESD_CHAR_DEVICE // change it later
    if(file == NULL)
    {
        perror("Unable to open or create the file");
        return -1;
    }  
    #else
    if(file < 0)
    {
        perror("Unable to open or create the file");
        return -1;
    }  
    close(file);
    #endif

    openlog("aesdsocket.c", LOG_CONS | LOG_PID, LOG_USER);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        syslog(LOG_ERR, "Unable to create TCP Socket");
        perror("Unable to create TCP Socket\n");
        #ifndef USE_AESD_CHAR_DEVICE // change it later
        fclose(file);
        #else
        close(file);
        #endif
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
        #ifndef USE_AESD_CHAR_DEVICE // change it later
        fclose(file);
        #else
        close(file);
        #endif
        closelog();        
        return -1;
    }

    if(listen(sockfd, 5) == -1)
    {
        syslog(LOG_ERR, "Unable to listen at created TCP socket");
        perror("Unable to listen at created TCP socket\n");
        close(sockfd);
        #ifndef USE_AESD_CHAR_DEVICE // change it later
        fclose(file);
        #else
        close(file);
        #endif
        closelog();        
        return -1;
    }

    if(deamonize == 1)
    {
            pid_t pid = fork();

            if(pid < 0)
            {
                printf("failed to fork\n"); 
                close(sockfd);
                #ifndef USE_AESD_CHAR_DEVICE // change it later
                fclose(file);
                #else
                close(file);
                #endif
                closelog();  
                exit(EXIT_FAILURE);      
            }   

            if(pid > 0 )
            {
                printf("Running as a deamon\n");
                exit(EXIT_SUCCESS);
            }

            umask(0);

            if(setsid() < 0)
            {
                printf("Failed to create SID for child\n");
                close(sockfd);
                #ifndef USE_AESD_CHAR_DEVICE // change it later
                fclose(file);
                #else
                close(file);
                #endif
                closelog(); 
                exit(EXIT_FAILURE);
            }

            if(chdir("/") < 0)
            {
                printf("Unable to change directory to root\n");
                close(sockfd);
                #ifndef USE_AESD_CHAR_DEVICE // change it later
                fclose(file);
                #else
                close(file);
                #endif
                closelog(); 
                exit(EXIT_FAILURE);
            }

            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);        
    }

    signal(SIGALRM, signalInterruptHandler);
    alarm(10);
    syslog(LOG_INFO, "TCP server listening at port %d", ntohs(addr.sin_port));
    //printf("TCP server listening at port %d\n", ntohs(addr.sin_port));

    int num_threads = 0;
    Node *head = NULL;
    
    while(1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if(client_sockfd == -1)
        {
            syslog(LOG_ERR, "Unable to accept the client's connection");
            perror("Unable to accept the client's connection\n");
            #ifndef USE_AESD_CHAR_DEVICE // change it later
            fclose(file);
            #else
            close(file);
            #endif
            closelog();            
            return -1;
        }      

        client_info_t *client_info = malloc(sizeof(client_info_t));
        if(client_info == NULL)
        {
            syslog(LOG_ERR, "Unable to allocate memory for client_info");
            perror("Unable to allocate memory for client_info");
            close(client_sockfd);
            continue;
        }      

        client_info->client_sockfd = client_sockfd;
        client_info->client_addr = client_addr;

        Node *n = malloc(sizeof(Node)); 
        if(n == NULL)
        {
            syslog(LOG_ERR, "Failed to allocate memory for thread");
            perror("Failed to allocate memory for thread\n");
            close(client_sockfd);
            free(client_info);
            continue;
        }

        if(pthread_create(&(n->tid), NULL, handle_client, (void *) client_info) != 0)
        {
            syslog(LOG_ERR, "Unable to create thread");
            perror("Unable to create thread");
            close(client_sockfd);
            free(client_info);
            continue;
        }

        n->next = head;
        head = n;

        num_threads++;
    }

    Node *current = head;
    Node *next;

    while(current != NULL)
    {
        if (pthread_join(current->tid, NULL) != 0) {
            printf("Failed to join thread \n");
        }
        current = current->next;
    }

    current = head;
    while(current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }

    close(sockfd);
    #ifndef USE_AESD_CHAR_DEVICE // change it later
    fclose(file);
    #else
    close(file);
    #endif
    closelog();               
    return 0; 
}

int main(int argc, char *argv[])
{
    int deamonize = 0;

    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[1], "-d") == 0)
        {
            deamonize = 1;
        }
    }

    if(createTCPServer(deamonize) == -1) printf("Error in running application\n");
    
    return 0;
}
