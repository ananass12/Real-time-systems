#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define EXAMPLE_SOCK_PATH "/tmp/example_resmgr.sock"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Использование: %s \"команда\"\n", argv[0]);
        fprintf(stderr, "Примеры:\n");
        fprintf(stderr, "  %s \"GET\"\n", argv[0]);
        fprintf(stderr, "  %s \"SET 42\"\n", argv[0]);
        fprintf(stderr, "  %s \"INC\"\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, EXAMPLE_SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(fd);
        return EXIT_FAILURE;
    }

    //отправляем команду + \n
    const char *msg = argv[1];
    size_t len = strlen(msg);
    char *msg_with_nl = malloc(len + 2); // +1 для \n, +1 для \0
    if (!msg_with_nl) {
        perror("malloc");
        close(fd);
        return EXIT_FAILURE;
    }
    memcpy(msg_with_nl, msg, len);
    msg_with_nl[len] = '\n';
    msg_with_nl[len + 1] = '\0';

    if (send(fd, msg_with_nl, len + 1, 0) != (ssize_t)(len + 1)) {
        perror("send");
        free(msg_with_nl);
        close(fd);
        return EXIT_FAILURE;
    }
    free(msg_with_nl);

    //получаем ответ
    char buf[1024];
    ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
    if (n < 0) {
        perror("recv");
        close(fd);
        return EXIT_FAILURE;
    }
    buf[n] = '\0';
    printf("%s", buf);  //ответ уже содержит \n

    close(fd);
    return EXIT_SUCCESS;
}