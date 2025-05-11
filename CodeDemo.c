#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

// Định nghĩa tên thiết bị và IOCTL commands
#define DEVICE_PATH "/dev/tcs34725"
#define TCS34725_IOCTL_MAGIC 't'
#define TCS34725_IOCTL_READ_COLOR _IOR(TCS34725_IOCTL_MAGIC, 1, int)

int main() {
    int fd;
    int color_data;

    // open
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open the device");
        return errno;
    }

    if (ioctl(fd, TCS34725_IOCTL_READ_COLOR, &color_data) < 0) {
        perror("Failed to read color data");
        close(fd);
        return errno;
    }

    printf("Color Data (Clear): %d\n", color_data);

    // read red
    if (ioctl(fd, TCS34725_IOCTL_READ_COLOR, &color_data) < 0) {
        perror("Failed to read red color data");
        close(fd);
        return errno;
    }
    printf("Color Data (Red): %d\n", color_data);

    // read green
    if (ioctl(fd, TCS34725_IOCTL_READ_COLOR, &color_data) < 0) {
        perror("Failed to read green color data");
        close(fd);
        return errno;
    }
    printf("Color Data (Green): %d\n", color_data);

    // read blue
    if (ioctl(fd, TCS34725_IOCTL_READ_COLOR, &color_data) < 0) {
        perror("Failed to read blue color data");
        close(fd);
        return errno;
    }
    printf("Color Data (Blue): %d\n", color_data);

    close(fd);
    return 0;
}
