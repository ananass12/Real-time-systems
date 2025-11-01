// Демонстрация гонки данных (race condition) между потоками
// Без использования мьютексов или других средств синхронизации

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#define NumThreads 16

// Общие переменные, доступные всем потокам
volatile int var1 = 0;
volatile int var2 = 0;

// Флаг для корректного завершения потоков
volatile sig_atomic_t stop_flag = 0;

char *progname = "nomutex";

// Функция, выполняемая в каждом потоке
void *update_thread(void *arg) {
    long thread_id = (long)arg;

    while (!stop_flag) {
        var1++;
        var2++;

        //Проверяем, не нарушилось ли условие согласованности
        if (var1 != var2) {
            printf("%s: RACE DETECTED! Thread %ld: var1=%d, var2=%d\n",
                   progname, thread_id, var1, var2);
            exit(1);  // Завершаем программу сразу при обнаружении гонки
        }
    }

    return NULL;
}

int main(void) {
    pthread_t threadID[NumThreads];
    int i;

    //Включаем построчный буфер вывода для немедленного отображения printf
    setvbuf(stdout, NULL, _IOLBF, 0);

    printf("%s: starting %d threads...\n", progname, NumThreads);

    //Создаём потоки (без специальных атрибутов — используем настройки по умолчанию)
    for (i = 0; i < NumThreads; i++) {
        if (pthread_create(&threadID[i], NULL, update_thread, (void *)(long)i) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    //Даём потокам поработать 1–2 секунды
    sleep(2);

    //Устанавливаем флаг завершения
    stop_flag = 1;

    //Ждём завершения всех потоков
    for (i = 0; i < NumThreads; i++) {
        pthread_join(threadID[i], NULL);
    }

    printf("%s: all done. Final values: var1=%d, var2=%d\n", progname, var1, var2);
    return 0;
}