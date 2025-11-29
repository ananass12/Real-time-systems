#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include <unistd.h>

#define ARRAY_SIZE (512UL * 1024 * 1024) // 512 MB
#define PAGE_SIZE 4096
#define NUM_ITERATIONS 1000

static long long timespec_diff_ns(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) * 1000000000LL + (b.tv_nsec - a.tv_nsec);
}

int main(void) {
    printf("Task 1: Demonstration of Page Faults\n");

    char *array = malloc(ARRAY_SIZE);
    if (!array) {
        perror("malloc");
        return 1;
    }

    // Открываем файл для записи
    FILE *data_file = fopen("task1_data.csv", "w");
    if (!data_file) {
        perror("fopen");
        free(array);
        return 1;
    }
    
    // Записываем заголовок в CSV файл
    fprintf(data_file, "Iteration,Latency_ns,MinorFaults,MajorFaults\n");

    struct timespec t1, t2;
    struct rusage ru_before, ru_after;

    printf("Iter\tLatency_ns\tMinorDiff\tMajorDiff\n");
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // sample rusage before access
        getrusage(RUSAGE_SELF, &ru_before);
        clock_gettime(CLOCK_MONOTONIC, &t1);

        size_t index = ((size_t)i * PAGE_SIZE) % ARRAY_SIZE;
        array[index] = (char)i; // write to page

        clock_gettime(CLOCK_MONOTONIC, &t2);
        getrusage(RUSAGE_SELF, &ru_after);

        long long lat = timespec_diff_ns(t1, t2);
        long minor = ru_after.ru_minflt - ru_before.ru_minflt;
        long major = ru_after.ru_majflt - ru_before.ru_majflt;

        // Вывод в консоль
        printf("%d\t%lld\t%ld\t%ld\n", i, lat, minor, major);
        fflush(stdout);
        
        // Запись в файл
        fprintf(data_file, "%d,%lld,%ld,%ld\n", i, lat, minor, major);
        fflush(data_file);
    }

    fclose(data_file);
    free(array);
    
    printf("Data saved to task1_data.csv\n");
    return 0;
}