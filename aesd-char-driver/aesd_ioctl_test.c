#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "aesd_ioctl.h"

int main() {
    int fd;
    struct aesd_seekto seekto = {
        .write_cmd = 115,
        .write_cmd_offset = 200
    };

    fd = open("/dev/aesdchar", O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device");
        return -1;
    }

    if (ioctl(fd, AESDCHAR_IOCSEEKTO, &seekto) == -1) {
        perror("Failed to perform ioctl");
        close(fd);
        return -1;
    }

    printf("ioctl executed successfully\n");

    close(fd);
    return 0;
}
