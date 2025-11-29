#include "mempool.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>

// Узел в связном списке свободных блоков
typedef struct Node {
    struct Node* next;
} Node;

// Структура, описывающая пул
struct MemoryPool {
    size_t block_size;
    size_t block_count;
    Node* free_list_head; 
    void* memory_start;    
    size_t memory_total_size;
};

MemoryPool* pool_create(size_t block_size, size_t block_count) {
    if (block_count == 0) return NULL;
    // Размер блока должен быть достаточным, чтобы вместить указатель Node
    if (block_size < sizeof(Node)) {
        block_size = sizeof(Node);
    }

    // Выделить память для самой структуры пула
    MemoryPool* pool = (MemoryPool*)malloc(sizeof(MemoryPool));
    if (!pool) return NULL;

    pool->block_size = ((block_size + (sizeof(void*) - 1)) / sizeof(void*)) * sizeof(void*);
    pool->block_count = block_count;
    pool->memory_total_size = pool->block_size * block_count;

    // Выделить один большой кусок памяти для всех блоков
    pool->memory_start = malloc(pool->memory_total_size);
    if (!pool->memory_start) {
        free(pool);
        return NULL;
    }

    // Инициализируем память нулями (опционально)
    memset(pool->memory_start, 0, pool->memory_total_size);

    // Попытка заблокировать память
    if (mlock(pool->memory_start, pool->memory_total_size) != 0) {
        // Возможна ошибка из-за прав — не фатально, но нужно уведомить
        perror("mlock failed (continuing without mlock)");
        // Мы не возвращаем ошибку — пул всё равно работает, но без гарантий от major faults
    }

    // Построить список свободных блоков
    pool->free_list_head = NULL;
    for (size_t i = 0; i < block_count; ++i) {
        Node* node = (Node*)((char*)pool->memory_start + i * pool->block_size);
        node->next = pool->free_list_head;
        pool->free_list_head = node;
    }

    return pool;
}

void* pool_alloc(MemoryPool* pool) {
    if (!pool) return NULL;
    Node* n = pool->free_list_head;
    if (!n) return NULL; // пустой пул
    pool->free_list_head = n->next;
    return (void*)n;
}

void pool_free(MemoryPool* pool, void* block) {
    if (!pool || !block) return;
    Node* n = (Node*)block;
    n->next = pool->free_list_head;
    pool->free_list_head = n;
}

void pool_destroy(MemoryPool* pool) {
    if (!pool) return;
    // Попытка разблокировать
    if (pool->memory_start) {
        munlock(pool->memory_start, pool->memory_total_size);
        free(pool->memory_start);
    }
    free(pool);
}