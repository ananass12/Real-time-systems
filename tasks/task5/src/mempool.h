#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stddef.h>

typedef struct MemoryPool MemoryPool;

/**
* Создает пул памяти для блоков фиксированного размера.
* block_size -- размер блока в байтах (>= sizeof(void*)).
* block_count -- число блоков в пуле.
* Возвращает NULL при ошибке.
*/
MemoryPool* pool_create(size_t block_size, size_t block_count);

/* Выделить один блок (O(1)). Возвращает NULL если пул пуст. */
void* pool_alloc(MemoryPool* pool);

/* Вернуть блок в пул (O(1)). */
void pool_free(MemoryPool* pool, void* block);

/* Уничтожить пул, освободить память. */
void pool_destroy(MemoryPool* pool);

#endif // MEMPOOL_H
