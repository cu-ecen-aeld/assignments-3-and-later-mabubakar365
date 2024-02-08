#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<syslog.h>

int main(int argc, char *argv[])
{
    int fd;
    size_t bytesWritten = 0;

    openlog("writer.c", LOG_CONS | LOG_PID, LOG_USER);

    if(argc < 3)
    {
        syslog(LOG_ERR, "wrong number of parameters");
        // printf("wrong number of parameters\n");
        return 1;
    }

    else if(argv[1] == NULL || (strlen(argv[1]) == 0))
    {
        syslog(LOG_ERR, "parameter 1 is either NULL or empty");
        // printf("parameter 1 is either NULL or empty\n");
        return 1;
    }

    else if(argv[2] == NULL || (strlen(argv[2]) == 0))
    {
        syslog(LOG_ERR, "parameter 2 is either NULL or empty");
        // printf("parameter 2 is either NULL or empty\n");
        return 1;
    }

    else
    {
        // printf("arguments passed are as expected\n");
        // syslog(LOG_DEBUG, "arguments passed are as expected");
        // syslog(LOG_DEBUG, "argument 1 %s", argv[1]);
        // syslog(LOG_DEBUG, "argument 2 %s", argv[2]);
        // printf("argument 1 %s\n", argv[1]);
        // printf("argument 2 %s\n", argv[2]);
        fd = open(argv[1], O_WRONLY|O_CREAT, S_IWUSR | S_IRUSR);
        if(fd == -1)
        {
            syslog(LOG_ERR, "Error opening file %s\n", argv[1]);
            // printf("Error opening file %s\n", argv[1]);
            return 1;
        }

        bytesWritten = write(fd, argv[2], strlen(argv[2]));
        if(bytesWritten == -1)
        {
            syslog(LOG_ERR, "Error writing to file %s\n", argv[1]);
            // printf("Error writing to file %s\n", argv[1]);
            return 1;            
        }

        syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);
        // printf("wrote %ld bytes to the file %s\n", bytesWritten, argv[1]);
        closelog();
        close(fd);
    }

    return 0;
}