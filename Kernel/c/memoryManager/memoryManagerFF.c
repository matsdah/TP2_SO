// FF por First Fit -> algorimto de asignación de memoria (ver en el .md)
// Recorre la lista buscando el primer bloque libre con block->size >= size

#include "../include/memoryManager.h"

typedef struct MemBlock {
    uint64_t size;              // Tamano del bloque (sin contar el header)
    int is_free;                // 1 si esta libre, 0 si esta asignado
    struct MemBlock *next;      // Siguiente bloque en la lista
} MemBlock;

