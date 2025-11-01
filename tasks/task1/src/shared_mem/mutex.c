#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#define NumThreads 16

volatile int var1 = 0;
volatile int var2 = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

volatile sig_atomic_t stop_flag = 0;
char *progname = "mutex";

void *update_thread(void *arg) {
    long id = (long)arg;
    while (!stop_flag) {
        pthread_mutex_lock(&mutex);
        var1++;
        var2++;
        if (var1 != var2) {
            printf("Ошибка\n");
            exit(1);
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(void) {
    pthread_t threads[NumThreads];
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("%s: starting %d threads...\n", progname, NumThreads);

    for (int i = 0; i < NumThreads; i++) {
        pthread_create(&threads[i], NULL, update_thread, (void *)(long)i);
    }

    sleep(2);          // даём поработать
    stop_flag = 1;     // просим завершиться

    for (int i = 0; i < NumThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("%s: all done. var1 = %d, var2 = %d\n", progname, var1, var2);
    return 0;
}