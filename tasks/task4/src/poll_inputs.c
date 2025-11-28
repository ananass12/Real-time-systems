#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <linux/input.h>
#include <sys/ioctl.h>

#define MAX_DEVICES 16

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s /dev/input/eventX1 /dev/input/eventX2 ...\n", argv[0]);
        return 1;
    }

    if (argc - 1 > MAX_DEVICES) {
        fprintf(stderr, "Error: Too many devices. Max is %d.\n", MAX_DEVICES);
        return 1;
    }

    int num_devices = argc - 1;
    struct pollfd fds[MAX_DEVICES];
    char device_names[MAX_DEVICES][256];

    // Открыть все переданные устройства
    for (int i = 0; i < num_devices; i++) {
        const char *path = argv[i + 1];
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            if (errno == EACCES) {
                fprintf(stderr, "Error: Permission denied on %s\n", path);
            } else {
                perror("Failed to open device");
            }
            // Закрываем уже открытые дескрипторы
            for (int j = 0; j < i; j++) {
                close(fds[j].fd);
            }
            return 1;
        }

        fds[i].fd = fd;
        fds[i].events = POLLIN;
        fds[i].revents = 0;

        // Получаем имя устройства
        if (ioctl(fd, EVIOCGNAME(sizeof(device_names[i])), device_names[i]) < 0) {
            snprintf(device_names[i], sizeof(device_names[i]), "device%d", i);
        }
    }

    printf("Polling %d devices. Press Ctrl+C to exit.\n", num_devices);

    while (1) {
        int ret = poll(fds, num_devices, -1);
        if (ret < 0) {
            if (errno == EINTR) {
                continue; // Прервано сигналом (например, Ctrl+C)
            }
            perror("poll failed");
            break;
        }

        for (int i = 0; i < num_devices; i++) {
            if (fds[i].revents & POLLIN) {
                struct input_event ev;
                ssize_t bytes;
                while ((bytes = read(fds[i].fd, &ev, sizeof(ev))) == sizeof(ev)) {
                    // Выводим событие с именем устройства
                    printf("[%s] type=%d, code=%d, value=%d\n",
                           device_names[i], ev.type, ev.code, ev.value);
                }
                // read() вернёт 0 или -1, когда данных больше нет (благодаря O_NONBLOCK)
                if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("read error");
                }
            }
        }
    }

    // Закрыть все файловые дескрипторы
    for (int i = 0; i < num_devices; i++) {
        close(fds[i].fd);
    }

    return 0;
}
