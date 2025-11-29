#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mempool.h"

#define BENCH_ITERATIONS 1000000
#define BLOCK_SIZE 128

static long long timespec_diff_ns(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) * 1000000000LL + (b.tv_nsec - a.tv_nsec);
}

void bench_malloc_free(void) {
    printf("Benchmark: malloc/free x %d\n", BENCH_ITERATIONS);
    
    // Открываем файл для записи
    FILE *file = fopen("malloc_benchmark.csv", "w");
    if (!file) {
        perror("fopen malloc_benchmark.csv");
        return;
    }
    fprintf(file, "Iteration,Latency_ns\n");
    
    struct timespec t1, t2;
    long long max_ns = 0;
    long long total_ns = 0;

    for (int i = 0; i < BENCH_ITERATIONS; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &t1);
        void* p = malloc(BLOCK_SIZE);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        
        long long d = timespec_diff_ns(t1, t2);
        if (d > max_ns) max_ns = d;
        total_ns += d;
        
        // Запись в файл каждые 1000 итераций для уменьшения размера файла
        if (i % 1000 == 0) {
            fprintf(file, "%d,%lld\n", i, d);
        }
        
        free(p);
    }
    
    fclose(file);
    printf("malloc max latency: %lld ns\n", max_ns);
    printf("malloc avg latency: %lld ns\n", total_ns / BENCH_ITERATIONS);
    printf("malloc data saved to malloc_benchmark.csv\n");
}

void bench_pool(void) {
    printf("Benchmark: pool_alloc/pool_free x %d\n", BENCH_ITERATIONS);
    
    // Открываем файл для записи
    FILE *file = fopen("pool_benchmark.csv", "w");
    if (!file) {
        perror("fopen pool_benchmark.csv");
        return;
    }
    fprintf(file, "Iteration,Latency_ns\n");
    
    struct timespec t1, t2;
    long long max_ns = 0;
    long long total_ns = 0;

    MemoryPool* pool = pool_create(BLOCK_SIZE, 1024*1024); // >= BENCH_ITERATIONS
    if (!pool) {
        printf("Failed to create pool\n");
        fclose(file);
        return;
    }

    void** tmp = malloc(sizeof(void*) * BENCH_ITERATIONS);
    if (!tmp) {
        printf("Failed to allocate temp array\n");
        pool_destroy(pool);
        fclose(file);
        return;
    }

    // Тестируем alloc
    for (int i = 0; i < BENCH_ITERATIONS; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &t1);
        tmp[i] = pool_alloc(pool);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        
        long long d = timespec_diff_ns(t1, t2);
        if (d > max_ns) max_ns = d;
        total_ns += d;
        
        // Запись в файл каждые 1000 итераций
        if (i % 1000 == 0) {
            fprintf(file, "%d,%lld\n", i, d);
        }
    }

    // Тестируем free
    long long free_max_ns = 0;
    long long free_total_ns = 0;
    
    for (int i = 0; i < BENCH_ITERATIONS; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &t1);
        pool_free(pool, tmp[i]);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        
        long long d = timespec_diff_ns(t1, t2);
        if (d > free_max_ns) free_max_ns = d;
        free_total_ns += d;
    }

    free(tmp);
    pool_destroy(pool);
    fclose(file);
    
    printf("pool_alloc max latency: %lld ns\n", max_ns);
    printf("pool_alloc avg latency: %lld ns\n", total_ns / BENCH_ITERATIONS);
    printf("pool_free max latency: %lld ns\n", free_max_ns);
    printf("pool_free avg latency: %lld ns\n", free_total_ns / BENCH_ITERATIONS);
    printf("pool data saved to pool_benchmark.csv\n");
}

void bench_comprehensive(void) {
    printf("\nComprehensive benchmark with detailed statistics...\n");
    
    FILE *file = fopen("comprehensive_benchmark.csv", "w");
    if (!file) {
        perror("fopen comprehensive_benchmark.csv");
        return;
    }
    fprintf(file, "Operation,Iteration,Latency_ns\n");
    
    // Тестируем malloc/free
    struct timespec t1, t2;
    long long malloc_max = 0, malloc_total = 0;
    long long free_max = 0, free_total = 0;
    
    for (int i = 0; i < 100000; ++i) { // Меньше итераций для подробного теста
        // malloc
        clock_gettime(CLOCK_MONOTONIC, &t1);
        void* p = malloc(BLOCK_SIZE);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        long long malloc_lat = timespec_diff_ns(t1, t2);
        
        if (malloc_lat > malloc_max) malloc_max = malloc_lat;
        malloc_total += malloc_lat;
        
        // free
        clock_gettime(CLOCK_MONOTONIC, &t1);
        free(p);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        long long free_lat = timespec_diff_ns(t1, t2);
        
        if (free_lat > free_max) free_max = free_lat;
        free_total += free_lat;
        
        if (i % 1000 == 0) {
            fprintf(file, "malloc,%d,%lld\n", i, malloc_lat);
            fprintf(file, "free,%d,%lld\n", i, free_lat);
        }
    }
    
    // Тестируем pool
    MemoryPool* pool = pool_create(BLOCK_SIZE, 100000);
    if (pool) {
        void* blocks[100000];
        
        long long pool_alloc_max = 0, pool_alloc_total = 0;
        long long pool_free_max = 0, pool_free_total = 0;
        
        for (int i = 0; i < 100000; ++i) {
            // pool_alloc
            clock_gettime(CLOCK_MONOTONIC, &t1);
            blocks[i] = pool_alloc(pool);
            clock_gettime(CLOCK_MONOTONIC, &t2);
            long long alloc_lat = timespec_diff_ns(t1, t2);
            
            if (alloc_lat > pool_alloc_max) pool_alloc_max = alloc_lat;
            pool_alloc_total += alloc_lat;
            
            // pool_free
            clock_gettime(CLOCK_MONOTONIC, &t1);
            pool_free(pool, blocks[i]);
            clock_gettime(CLOCK_MONOTONIC, &t2);
            long long free_lat = timespec_diff_ns(t1, t2);
            
            if (free_lat > pool_free_max) pool_free_max = free_lat;
            pool_free_total += free_lat;
            
            if (i % 1000 == 0) {
                fprintf(file, "pool_alloc,%d,%lld\n", i, alloc_lat);
                fprintf(file, "pool_free,%d,%lld\n", i, free_lat);
            }
        }
        
        pool_destroy(pool);
        
        printf("\n--- Comprehensive Results ---\n");
        printf("malloc:  max=%lldns, avg=%lldns\n", malloc_max, malloc_total / 100000);
        printf("free:    max=%lldns, avg=%lldns\n", free_max, free_total / 100000);
        printf("pool_alloc: max=%lldns, avg=%lldns\n", pool_alloc_max, pool_alloc_total / 100000);
        printf("pool_free:  max=%lldns, avg=%lldns\n", pool_free_max, pool_free_total / 100000);
    }
    
    fclose(file);
    printf("Comprehensive data saved to comprehensive_benchmark.csv\n");
}

int main(void) {
    printf("=== Memory Pool Benchmark ===\n");
    
    // Основные бенчмарки
    bench_malloc_free();
    bench_pool();
    
    // Детальный бенчмарк
    bench_comprehensive();
    
    return 0;
}