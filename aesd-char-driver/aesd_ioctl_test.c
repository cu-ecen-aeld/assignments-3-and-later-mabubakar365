#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "aesd_ioctl.h"

int main() {
    int fd;
    char buffer[20]; // Buffer to hold each command string
    struct aesd_seekto seekto = {
        .write_cmd = 1,
        .write_cmd_offset = 4
    };

    fd = open("/dev/aesdchar", O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device");
        return -1;
    }

    for (int i = 1; i <= 10; i++) {
        // Generate command string
        snprintf(buffer, sizeof(buffer), "write%d\n", i);
        
        // Write the command to the device
        if (write(fd, buffer, sizeof(buffer)) == -1) {
            perror("Failed to write to the device");
            close(fd);
            return -1;
        }
    }

    // Perform ioctl after writing commands
    if (ioctl(fd, AESDCHAR_IOCSEEKTO, &seekto) == -1) {
        perror("Failed to perform ioctl");
        close(fd);
        return -1;
    }

    printf("Commands written and ioctl executed successfully\n");

    close(fd);
    return 0;
}
