#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s /dev/input/eventX\n", argv[0]);
        return 1;
    }

    const char *device_path = argv[1];

    // Открыть файл устройства для чтения (O_RDONLY)
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    /* --- ЗАДАНИЕ 2: ИСПОЛЬЗОВАНИЕ IOCTL --- */
    // Создать буфер для имени устройства
    // Использовать ioctl с EVIOCGNAME для получения имени
    // Вывести имя устройства
    char name[256] = "Unknown";
    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
        perror("Warning: ioctl(EVIOCGNAME) failed");
        // Продолжаем работу, имя останется "Unknown"
    } else {
        printf("Device name: %s\n", name);
    }

    //Дополнительно физический путь
    char phys[256] = "Unknown";
    if (ioctl(fd, EVIOCGPHYS(sizeof(phys)), phys) < 0) {
        // Не критично, многие устройства не выставляют phys
    } else {
        printf("Physical path: %s\n", phys);
    }

    printf("Reading events from %s. Press Ctrl+C to exit.\n", device_path);

    struct input_event ev;
    while (1) {
        ssize_t bytes = read(fd, &ev, sizeof(ev));
        if (bytes != sizeof(ev)) {
            if (bytes < 0 && errno == EINTR) {
                continue; // Прервано сигналом — продолжаем
            }
            perror("Failed to read event");
            break;
        }

        // Выводим все события
        printf("Event: type %d, code %d, value %d\n", ev.type, ev.code, ev.value);
    }

    close(fd);
    return 0;
}
