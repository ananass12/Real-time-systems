#ifndef SHM_COMMON_H
#define SHM_COMMON_H

#include <stdint.h>

// Имена для объектов ядра (shared memory и семафоры)
// Начинаем с / для переносимости между системами.
#define SHM_NAME        "/shm_example"
#define SEM_PRODUCER    "/sem_producer_ex"
#define SEM_CONSUMER    "/sem_consumer_ex"

#define BUFFER_SIZE     10   //маленький размер для демонстрации, удобно наблюдать переполнение

typedef struct {
    uint64_t buffer[BUFFER_SIZE]; //кольцевой буфер для хранения данных между producer и consumer
    int head; // указывает на позицию, куда producer будет записывать следующий элемент
    int tail; // указывает на позицию, откуда consumer будет читать
} shared_data_t;

#endif // SHM_COMMON_H
