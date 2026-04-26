// FF por First Fit -> algorimto de asignación de memoria (ver en el .md)
// Recorre la lista buscando el primer bloque libre con block->size >= size

#include "../include/memoryManager.h"
#include <stddef.h>

typedef struct MemBlock {
    uint64_t size;              // Tamano del bloque (sin contar el header)
    int is_free;                // 1 si esta libre, 0 si esta asignado
    struct MemBlock *next;      // Siguiente bloque en la lista
} MemBlock;

// Tamano minimo de payload restante para que valga la pena splitear
#define MIN_SPLIT (sizeof(MemBlock) + 8)

static MemBlock *heap_start = NULL;

void mm_init(void *start, uint64_t size) {
    if (size <= sizeof(MemBlock))
        return;
    heap_start = (MemBlock *)start;
    heap_start->size = size - sizeof(MemBlock);
    heap_start->is_free = 1;
    heap_start->next = NULL;
}

void *mm_malloc(uint64_t size) {
    if (size == 0 || heap_start == NULL)
        return NULL;

    // Alinear a 8 bytes para evitar accesos desalineados
    size = (size + 7) & ~(uint64_t)7;

    MemBlock *block = heap_start;
    while (block != NULL) {
        if (block->is_free && block->size >= size) {
            // Split si sobra suficiente espacio para un bloque nuevo
            if (block->size >= size + MIN_SPLIT) {
                MemBlock *split = (MemBlock *)((uint8_t *)(block + 1) + size);
                split->size = block->size - size - sizeof(MemBlock);
                split->is_free = 1;
                split->next = block->next;
                block->size = size;
                block->next = split;
            }
            block->is_free = 0;
            return (void *)(block + 1);
        }
        block = block->next;
    }
    return NULL;
}

void mm_free(void *ptr) {
    if (ptr == NULL)
        return;

    MemBlock *block = (MemBlock *)ptr - 1;
    block->is_free = 1;

    // Coalescing hacia adelante: absorber bloques libres consecutivos
    while (block->next != NULL && block->next->is_free) {
        block->size += sizeof(MemBlock) + block->next->size;
        block->next = block->next->next;
    }

    // Coalescing hacia atras: si el bloque anterior esta libre, absorber este
    MemBlock *prev = heap_start;
    while (prev != NULL && prev->next != block)
        prev = prev->next;
    if (prev != NULL && prev->is_free) {
        prev->size += sizeof(MemBlock) + block->size;
        prev->next = block->next;
    }
}

void mm_status(MemStatus *status) {
    if (status == NULL || heap_start == NULL)
        return;

    status->total = 0;
    status->used = 0;
    status->free = 0;
    status->alloc_count = 0;

    MemBlock *block = heap_start;
    while (block != NULL) {
        status->total += block->size;
        if (block->is_free) {
            status->free += block->size;
        } else {
            status->used += block->size;
            status->alloc_count++;
        }
        block = block->next;
    }
}
