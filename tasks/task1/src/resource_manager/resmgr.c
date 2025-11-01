/*
 Менеджер ресурсов 
 
 Реализовано простое устройство-счётчик с командами:
 GET - получить текущее значение
 SET N - установить значение N
 INC - увеличить на 1
 
 Права доступа - только первый клиент может писать (SET/INC), все остальные - только читать (GET)
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdbool.h>

#define EXAMPLE_SOCK_PATH "/tmp/example_resmgr.sock"

static const char *progname = "example";
static int optv = 0;
static int listen_fd = -1;

//глобальное состояние устройства
static int device_value = 0;
static bool first_client_granted_write = false;  //true после первого подключения
static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

static void options(int argc, char *argv[]);
static void install_signals(void);
static void on_signal(int signo);
static void *client_thread(void *arg);

//вспомогательные функции обработки команд
static bool handle_command(int fd, const char *cmd, bool can_write);

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("%s: starting...\n", progname);
    options(argc, argv);
    install_signals();   //устанавливаем обработчики сигналов SIGINT и SIGTERM

    //создаем UNIX-потоковый сокет
    listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    //подготавливаем адрес сокета
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    //копируем путь, оставляя место для завершающего нуля
    strncpy(addr.sun_path, EXAMPLE_SOCK_PATH, sizeof(addr.sun_path) - 1);

    //удаляем старый файл сокета, если он существует (чтобы bind не упал)
    unlink(EXAMPLE_SOCK_PATH);

    //привязываем сокет к файлу
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    //переводим сокет в режим прослушивания (максимум 8 ожидающих подключений)
    if (listen(listen_fd, 8) == -1) {
        perror("listen");
        close(listen_fd);
        unlink(EXAMPLE_SOCK_PATH);
        return EXIT_FAILURE;
    }

    printf("%s: listening on %s\n", progname, EXAMPLE_SOCK_PATH);
    printf("Примеры команд:\n");
    printf("  echo 'GET' | nc -U %s\n", EXAMPLE_SOCK_PATH);
    printf("  echo 'SET 42' | nc -U %s\n", EXAMPLE_SOCK_PATH);
    printf("  echo 'INC' | nc -U %s\n", EXAMPLE_SOCK_PATH);

    while (1) {
        //ждем нового клиента
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd == -1) {
            if (errno == EINTR) continue;
            perror("accept");
            break;
        }

        if (optv) {
            printf("%s: io_open — новое подключение (fd=%d)\n", progname, client_fd);
        }

        //запускаем новый поток для обработки клиента
        pthread_t th;
        if (pthread_create(&th, NULL, client_thread, (void *)(long)client_fd) != 0) {
            perror("pthread_create");
            close(client_fd);
            continue;
        }
        //отделяем поток
        pthread_detach(th);
    }

    if (listen_fd != -1) close(listen_fd);
    unlink(EXAMPLE_SOCK_PATH);
    return EXIT_SUCCESS;
}

//обработка одного клиента
static void *client_thread(void *arg)
{
    //получаем дескриптор клиента
    int fd = (int)(long)arg;
    char buf[1024];
    bool is_first_client = false;

    //определяем является ли клиент первым (и даем ему права на запись)
    pthread_mutex_lock(&device_mutex);
    if (!first_client_granted_write) {
        first_client_granted_write = true;
        is_first_client = true;
    }
    pthread_mutex_unlock(&device_mutex);

    if (optv) {
        printf("%s: клиент fd=%d %s правами на запись\n",
               progname, fd, is_first_client ? "получил" : "не получил");
    }

    //чтение команд от клиента
    for (;;) {
        ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n == 0) {
            if (optv) printf("%s: клиент закрыл соединение (fd=%d)\n", progname, fd);
            break;
        }
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recv");
            break;
        }

        buf[n] = '\0'; //обнуляем для корректной работы со строками

        if (optv) {
            printf("%s: io_read — %zd байт: \"%s\"\n", progname, n, buf);
        }

        //обрабатываем команду
        if (!handle_command(fd, buf, is_first_client)) {
            //отправлено "exit" или ошибка - завершаем
            break;
        }
    }

    close(fd);
    return NULL;
}

//обработка команды от клиента
static bool handle_command(int fd, const char *cmd, bool can_write)
{
    char response[256];
    int len = 0;

    //убираем завершающие \r\n
    char *newline = strpbrk((char *)cmd, "\r\n");
    if (newline) *newline = '\0';

    if (strcmp(cmd, "GET") == 0) {
        pthread_mutex_lock(&device_mutex);
        len = snprintf(response, sizeof(response), "%d\n", device_value);
        pthread_mutex_unlock(&device_mutex);
        send(fd, response, len, 0);
        return true;
    }

    if (strcmp(cmd, "INC") == 0) {
        if (!can_write) {
            send(fd, "Отказано в доступе к записи\n", 30, 0);
            return true;
        }
        pthread_mutex_lock(&device_mutex);
        device_value++;
        len = snprintf(response, sizeof(response), "OK (сейчас %d)\n", device_value);
        pthread_mutex_unlock(&device_mutex);
        send(fd, response, len, 0);
        return true;
    }

    if (strncmp(cmd, "SET ", 4) == 0) {
        if (!can_write) {
            send(fd, "Отказано в доступе к записи\n", 30, 0);
            return true;
        }
        int val;
        if (sscanf(cmd + 4, "%d", &val) == 1) {
            pthread_mutex_lock(&device_mutex);
            device_value = val;
            len = snprintf(response, sizeof(response), "OK\n");
            pthread_mutex_unlock(&device_mutex);
            send(fd, response, len, 0);
        } else {
            send(fd, "Неправильный номер\n", 22, 0);
        }
        return true;
    }

    if (strcmp(cmd, "exit") == 0) {
        return false;   //завершение
    }

    send(fd, "Неизвестная команда (используйте GET, SET N, INC, exit)\n", 51, 0);
    return true;
}

//аргументы командной строки
static void options(int argc, char *argv[])
{
    int opt;
    optv = 0;
    //поддерживаем только флаг -v (verbose)
    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
            case 'v':
                optv++;
                break;
        }
    }
}

//установка обработчика сигналов завершения
static void install_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;  //указываем функцию-обработчик
    sigemptyset(&sa.sa_mask);
    //регистрируем обработчик для SIGINT (Ctrl+C) и SIGTERM
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

//обработчик сигналов завершения
static void on_signal(int signo)
{
    (void)signo;
    if (listen_fd != -1) close(listen_fd);
    unlink(EXAMPLE_SOCK_PATH);
    fprintf(stderr, "\n%s: завершение по сигналу\n", progname);
    _exit(0);
}