/*
 * Демонстрация векторного (scatter-gather) I/O с readv/writev
 *
 * Цель: Показать, как можно записать или прочитать несколько
 * отдельных буферов в памяти за один системный вызов, избегая
 * необходимости предварительно копировать их в единый смежный буфер.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <stdint.h>
#include <arpa/inet.h> // для ntohl/htonl

// Структура сообщения
typedef struct {
    uint32_t msg_type;      // Тип сообщения
    uint64_t msg_id;        // Идентификатор сообщения
    char payload[20];       // Данные
} message_t;

int main() {
    int pipe_fd[2];

    // Создаём pipe для демонстрации записи и чтения
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    printf("--- WRITER ---\n");

    // Заполняем структуру
    message_t msg_to_send = {
        .msg_type = htonl(1), // Тип "DATA", преобразуем в сетевой порядок байт
        .msg_id = 9988776655443322L,
    };
    strncpy(msg_to_send.payload, "Hello, IOV!", sizeof(msg_to_send.payload));
    
    printf("Preparing to send:\n");
    printf("  msg_type: %u\n", ntohl(msg_to_send.msg_type));
    printf("  msg_id:   %llu\n", (unsigned long long)msg_to_send.msg_id);
    printf("  payload:  \"%s\"\n", msg_to_send.payload);

    // Настройка iovec для scatter-gather
    // Каждый элемент указывает на отдельное поле структуры
    struct iovec iov_write[3];
    iov_write[0].iov_base = &msg_to_send.msg_type;
    iov_write[0].iov_len = sizeof(msg_to_send.msg_type);
    
    iov_write[1].iov_base = &msg_to_send.msg_id;
    iov_write[1].iov_len = sizeof(msg_to_send.msg_id);
    
    iov_write[2].iov_base = msg_to_send.payload;
    iov_write[2].iov_len = strlen(msg_to_send.payload);  // можно отправить только реальную длину

    // Отправка всех частей структуры одним вызовом
    ssize_t bytes_written = writev(pipe_fd[1], iov_write, 3);
    if (bytes_written == -1) {
        perror("writev");
        exit(EXIT_FAILURE);
    }
    printf("writev() wrote %zd bytes.\n\n", bytes_written);
    close(pipe_fd[1]);      // Закрываем конец записи


    printf("--- READER ---\n");
    message_t msg_received = {0};   // структура для приёма данных

    // Настраиваем iovec для чтения в отдельные поля структуры
    struct iovec iov_read[3];
    iov_read[0].iov_base = &msg_received.msg_type;
    iov_read[0].iov_len = sizeof(msg_received.msg_type);

    iov_read[1].iov_base = &msg_received.msg_id;
    iov_read[1].iov_len = sizeof(msg_received.msg_id);

    iov_read[2].iov_base = msg_received.payload;
    iov_read[2].iov_len = strlen(msg_to_send.payload);      // длина известна из отправителя

    // Чтение всех частей структуры одним вызовом
    ssize_t bytes_read = readv(pipe_fd[0], iov_read, 3);
    if (bytes_read == -1) {
        perror("readv");
        exit(EXIT_FAILURE);
    }
    printf("readv() read %zd bytes.\n", bytes_read);

    // Вывод полученных данных
    printf("Received data:\n");
    printf("  msg_type: %u\n", ntohl(msg_received.msg_type));
    printf("  msg_id:   %llu\n", (unsigned long long)msg_received.msg_id);
    printf("  payload:  \"%s\"\n", msg_received.payload);
    
    close(pipe_fd[0]);      // Закрываем конец чтения

    // Проверка корректности
    if (ntohl(msg_received.msg_type) == 1 &&
        msg_received.msg_id == 9988776655443322L &&
        strcmp(msg_received.payload, "Hello, IOV!") == 0) {
        printf("\nCheck passed: data matches.\n");
    } else {
        printf("\nCheck failed: data does not match.\n");
    }

    return 0;
}

/*
* Выгода writev/readv:
* - Нет необходимости заранее копировать все части в единый буфер.
* - Система выполняет атомарную отправку/чтение сразу из нескольких буферов.
* - Удобно для структурированных данных и экономит ресурсы.
*/
