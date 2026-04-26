// Buddy System: asigna bloques de tamanio potencia de 2.
// Cada bloque tiene un header de 8 bytes con su order e is_free.
// Al liberar, se intenta coalescencia con el bloque "buddy" (par de la bifurcacion).

#include "../include/memoryManager.h"
#include <stddef.h>

#define MIN_ORDER 4     // 2^4 = 16 bytes (8B header + 8B payload minimo)
#define MAX_ORDER 23    // 2^23 = 8 MB

#define ORDERS (MAX_ORDER - MIN_ORDER + 1)

// Header presente en TODOS los bloques (libres y asignados)
typedef struct {
    uint8_t order;
    uint8_t is_free;
    uint8_t _pad[6];    // alinear payload a 8 bytes
} BlockHdr;             // 8 bytes

// Bloque libre: reutiliza los primeros bytes del payload para el enlace
typedef struct FreeNode {
    BlockHdr       hdr;
    struct FreeNode *next;
} FreeNode;

static void    *heap_base        = NULL;
static uint64_t heap_total       = 0;
static FreeNode *free_lists[ORDERS];
static uint64_t  alloc_count     = 0;

static inline int order_idx(int order) { return order - MIN_ORDER; }

// Menor order tal que 2^order >= size
static int size_to_order(uint64_t size) {
    int order = MIN_ORDER;
    while (((uint64_t)1 << order) < size) {
        if (++order > MAX_ORDER)
            return -1;
    }
    return order;
}

// Retorna el buddy de 'block' para el 'order' dado, o NULL si esta fuera del heap
static FreeNode *get_buddy(FreeNode *block, int order) {
    uintptr_t offset     = (uintptr_t)block - (uintptr_t)heap_base;
    uintptr_t buddy_off  = offset ^ ((uintptr_t)1 << order);
    if (buddy_off + ((uintptr_t)1 << order) > heap_total)
        return NULL;
    return (FreeNode *)((uintptr_t)heap_base + buddy_off);
}

static void list_remove(int order, FreeNode *target) {
    FreeNode **cur = &free_lists[order_idx(order)];
    while (*cur != NULL && *cur != target)
        cur = &(*cur)->next;
    if (*cur == target)
        *cur = target->next;
}

void mm_init(void *start, uint64_t size) {
    heap_base  = start;
    heap_total = size;
    alloc_count = 0;

    for (int i = 0; i < ORDERS; i++)
        free_lists[i] = NULL;

    // Descomponer el heap en bloques de potencia de 2 (greedy, mayor a menor)
    uintptr_t offset = 0;
    for (int order = MAX_ORDER; order >= MIN_ORDER && offset < size; order--) {
        uint64_t block_size = (uint64_t)1 << order;
        if (offset + block_size <= size) {
            FreeNode *block    = (FreeNode *)((uintptr_t)start + offset);
            block->hdr.order   = (uint8_t)order;
            block->hdr.is_free = 1;
            block->next        = free_lists[order_idx(order)];
            free_lists[order_idx(order)] = block;
            offset += block_size;
        }
    }
}

void *mm_malloc(uint64_t size) {
    if (size == 0 || heap_base == NULL)
        return NULL;

    int order = size_to_order(size + sizeof(BlockHdr));
    if (order < 0)
        return NULL;

    // Buscar la lista libre mas pequena con un bloque disponible
    int found = -1;
    for (int o = order; o <= MAX_ORDER; o++) {
        if (free_lists[order_idx(o)] != NULL) {
            found = o;
            break;
        }
    }
    if (found < 0)
        return NULL;

    // Extraer el bloque y splitear hacia abajo hasta el order pedido
    FreeNode *block = free_lists[order_idx(found)];
    free_lists[order_idx(found)] = block->next;

    while (found > order) {
        found--;
        // La mitad superior se convierte en buddy libre
        FreeNode *buddy    = (FreeNode *)((uintptr_t)block + ((uintptr_t)1 << found));
        buddy->hdr.order   = (uint8_t)found;
        buddy->hdr.is_free = 1;
        buddy->next        = free_lists[order_idx(found)];
        free_lists[order_idx(found)] = buddy;
        // La mitad inferior sigue el ciclo de split
        block->hdr.order = (uint8_t)found;
    }

    block->hdr.is_free = 0;
    alloc_count++;
    return (void *)((uintptr_t)block + sizeof(BlockHdr));
}

void mm_free(void *ptr) {
    if (ptr == NULL)
        return;

    FreeNode *block    = (FreeNode *)((uintptr_t)ptr - sizeof(BlockHdr));
    int       order    = block->hdr.order;
    block->hdr.is_free = 1;
    alloc_count--;

    // Coalescencia: absorber buddies libres del mismo order
    while (order < MAX_ORDER) {
        FreeNode *buddy = get_buddy(block, order);
        if (buddy == NULL || !buddy->hdr.is_free || buddy->hdr.order != order)
            break;
        list_remove(order, buddy);
        // El bloque fusionado empieza en la menor direccion
        if ((uintptr_t)buddy < (uintptr_t)block)
            block = buddy;
        order++;
        block->hdr.order = (uint8_t)order;
    }

    block->next = free_lists[order_idx(order)];
    free_lists[order_idx(order)] = block;
}

void mm_status(MemStatus *status) {
    if (status == NULL || heap_base == NULL)
        return;

    uint64_t free_bytes = 0;
    for (int i = 0; i < ORDERS; i++) {
        uint64_t block_size = (uint64_t)1 << (MIN_ORDER + i);
        FreeNode *cur = free_lists[i];
        while (cur != NULL) {
            free_bytes += block_size;
            cur = cur->next;
        }
    }

    status->total       = heap_total;
    status->free        = free_bytes;
    status->used        = heap_total - free_bytes;
    status->alloc_count = alloc_count;
}
