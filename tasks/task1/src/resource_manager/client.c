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
    if (argc < 2) {  //проверяем, что пользователь передал хотя бы одну команду
        fprintf(stderr, "Использование: %s \"команда\"\n", argv[0]);
        fprintf(stderr, "Примеры:\n");
        fprintf(stderr, "  %s \"GET\"\n", argv[0]);
        fprintf(stderr, "  %s \"SET 42\"\n", argv[0]);
        fprintf(stderr, "  %s \"INC\"\n", argv[0]);
        return EXIT_FAILURE;
    }

    //создаем UNIX-сокет типа потокового
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    //подготавливаем структуру адреса для подключения к серверу
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, EXAMPLE_SOCK_PATH, sizeof(addr.sun_path) - 1);

    //подключаемся к серверу по указанному сокету
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(fd);
        return EXIT_FAILURE;
    }

    //берем команду из аргумента командной строки
    const char *msg = argv[1];
    size_t len = strlen(msg);

    //выделяем память для команды с добавлением символа новой строки \n
    //+1 для \n, +1 для завершающего нуля \0
    char *msg_with_nl = malloc(len + 2);
    if (!msg_with_nl) {
        perror("malloc");    //не хватает памяти
        close(fd);
        return EXIT_FAILURE;
    }

    //копируем исходную команду
    memcpy(msg_with_nl, msg, len);
    msg_with_nl[len] = '\n';     //сервер ожидает команду завершённую \n
    msg_with_nl[len + 1] = '\0';   //завершаем строку 0

    //отправляем команду на сервер, ровно len + 1 байт (включая \n, но не включая завершающий \0)
    if (send(fd, msg_with_nl, len + 1, 0) != (ssize_t)(len + 1)) {
        perror("send");
        free(msg_with_nl);
        close(fd);
        return EXIT_FAILURE;
    }
    free(msg_with_nl);

    //буфер для получения ответа от сервера
    char buf[1024];
    //читаем ответ от сервера (максимум 1023 байта + 1 для \0)
    ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
    if (n < 0) {
        perror("recv");
        close(fd);
        return EXIT_FAILURE;
    }

    //завершаем полученную строку 0
    buf[n] = '\0';
    //выводим ответ сервера на экран 
    printf("%s", buf);  //ответ уже содержит \n

    close(fd);
    return EXIT_SUCCESS;
}