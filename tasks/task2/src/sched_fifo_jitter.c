/*
 Измерение джиттера при периодических пробуждениях с интервалом 2 мс под SCHED_FIFO.
*/

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#ifndef __linux__
int main(void) {
    printf("sched_fifo_jitter: пример только для Linux (SCHED_FIFO недоступен)\n");
    return 0;
}
#else

static int compare_i64(const void *a, const void *b) {
    int64_t va = *(const int64_t *)a;
    int64_t vb = *(const int64_t *)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

static inline int64_t ts_to_ns(const struct timespec *ts) {
    return (int64_t)ts->tv_sec * 1000000000LL + (int64_t)ts->tv_nsec;
}
static inline void ns_to_ts(int64_t ns, struct timespec *ts) {
    ts->tv_sec = (time_t)(ns / 1000000000LL);
    ts->tv_nsec = (long)(ns % 1000000000LL);
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    //1. Установка политики планировщика SCHED_FIFO
    // Переводим поток в режим реального времени с приоритетом 50.
    // Это позволяет потоку вытеснять все обычные (SCHED_OTHER) задачи.
    struct sched_param sp = {.sched_priority = 50};
    if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0) {
        perror("ПРЕДУПРЕЖДЕНИЕ: не удалось установить SCHED_FIFO; продолжаем с обычным планировщиком");
    } else {
        printf("Установлена политика SCHED_FIFO с приоритетом %d\n", sp.sched_priority);
    }

    //2. Блокировка всей памяти процесса
    // mlockall(MCL_CURRENT | MCL_FUTURE) предотвращает выгрузку страниц памяти в swap.
    // Page fault во время критического участка может вызвать задержки в миллисекунды.
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        perror("ПРЕДУПРЕЖДЕНИЕ: не удалось заблокировать память (mlockall)");
    } else {
        printf("Память процесса заблокирована (нельзя выгружать в swap)\n");
    }

    //3. Привязка потока к одному ядру CPU
    // Предотвращает миграцию потока между ядрами, что сохраняет кэш и TLB, уменьшая латентность и джиттер.
    long n_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (n_cpus > 0) {
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        // Привязываем к последнему ядру — оно часто менее загружено системными задачами.
        CPU_SET(n_cpus - 1, &cpu_set);
        if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set) != 0) {
            perror("ПРЕДУПРЕЖДЕНИЕ: не удалось установить привязку к CPU");
        } else {
            printf("Поток привязан к ядру CPU %ld\n", n_cpus - 1);
        }
    }

    //Основной цикл измерения джиттера
    const int64_t period = 2 * 1000000LL; // 2 мс = 2000000 нс
    const int samples = 5000;
    int64_t deltas[samples]; // Массив для хранения отклонений (джиттера)

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);
    int64_t next_ns = ts_to_ns(&next) + period;

    for (int i = 0; i < samples; ++i) {
        ns_to_ts(next_ns, &next);
        int rc;
        // Абсолютное ожидание 
        do {
            rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
        } while (rc == EINTR);
        if (rc != 0) {
            fprintf(stderr, "clock_nanosleep: %s\n", strerror(rc));
            return EXIT_FAILURE;
        }

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        // Джиттер = фактическое время пробуждения - ожидаемое время
        deltas[i] = ts_to_ns(&now) - next_ns;
        next_ns += period;
    }

    //Расчет статистики
    qsort(deltas, samples, sizeof(int64_t), compare_i64);
    int64_t min = deltas[0];
    int64_t max = deltas[samples - 1];
    int64_t p99 = deltas[(samples * 99) / 100];
    int64_t sum = 0;
    for (int i = 0; i < samples; ++i) {
        sum += deltas[i];
    }
    double avg = (double)sum / (double)samples;

    printf("\nСтатистика джиттера по %d измерениям (период 2 мс):\n", samples);
    printf("Минимальная задержка: %" PRId64 " нс\n", min);
    printf("Средняя задержка: %.1f нс\n", avg);
    printf("99-й перцентиль: %" PRId64 " нс\n", p99);
    printf("Максимальная задержка: %" PRId64 " нс\n", max);

    /* До:
       Джиттер мог достигать десятков или сотен микросекунд из-за:
         * вытеснения обычным планировщиком,
         * page faults при обращении к новой памяти,
         * миграции между ядрами с потерей кэша.
       После:
       - SCHED_FIFO гарантирует немедленное пробуждение (если нет других RT-потоков)
       - mlockall устраняет page faults - нет задержек от диска/swap.
       - Привязка к CPU сохраняет кэш L1/L2 и TLB - стабильное время доступа к памяти
       - Джиттер обычно снижается до сотен или даже десятков наносекунд
    */
    return 0;
}
#endif