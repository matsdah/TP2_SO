#include "scheduler.h"
#include "process.h"
#include "time.h"

// Leida desde interrupts.asm para decidir si hacer context switch voluntario
volatile uint64_t force_switch = 0;

static PCB *run_queue[MAX_PROCESSES];
static int  queue_size = 0;
static int  queue_idx  = 0;

void scheduler_init(void) {
    queue_size = 0;
    queue_idx  = 0;
}

void scheduler_add(PCB *p) {
    if (queue_size < MAX_PROCESSES)
        run_queue[queue_size++] = p;
}

void scheduler_remove(PCB *p) {
    for (int i = 0; i < queue_size; i++) {
        if (run_queue[i] == p) {
            run_queue[i] = run_queue[--queue_size];
            if (queue_idx >= queue_size && queue_size > 0)
                queue_idx = queue_size - 1;
            return;
        }
    }
}

// Elegir el siguiente proceso READY usando round-robin
PCB *scheduler_next_ready(void) {
    if (queue_size == 0) return NULL;

    for (int i = 0; i < queue_size; i++) {
        queue_idx = (queue_idx + 1) % queue_size;
        PCB *c = run_queue[queue_idx];
        if (c->state == PROCESS_READY || c->state == PROCESS_RUNNING)
            return c;
    }
    // Si no hay ninguno READY, devolver el primero de la cola (idle)
    return run_queue[0];
}

// ─── Llamada desde _irq00Handler (timer) ─────────────────────────────────────
uint64_t *scheduler_tick(uint64_t *current_rsp) {
    timer_handler();   // actualizar contador de ticks

    PCB *cur = process_current();
    if (cur != NULL) {
        cur->rsp = current_rsp;

        if (cur->state == PROCESS_RUNNING) {
            if (cur->remaining_quanta > 0)
                cur->remaining_quanta--;

            if (cur->remaining_quanta == 0) {
                cur->state            = PROCESS_READY;
                cur->remaining_quanta = cur->priority;
            }
        }
    }

    PCB *next = scheduler_next_ready();
    if (next == NULL) {
        // No hay procesos: devolver el mismo RSP (no cambiar nada)
        return current_rsp;
    }

    next->state = PROCESS_RUNNING;
    process_set_current(next);
    return next->rsp;
}

// ─── Llamada desde _irq128Handler cuando force_switch=1 ──────────────────────
uint64_t *scheduler_yield_impl(uint64_t *current_rsp) {
    PCB *cur = process_current();
    if (cur != NULL) {
        cur->rsp = current_rsp;
        if (cur->state == PROCESS_RUNNING)
            cur->state = PROCESS_READY;
    }

    PCB *next = scheduler_next_ready();
    if (next == NULL) return current_rsp;

    next->state = PROCESS_RUNNING;
    process_set_current(next);
    return next->rsp;
}

// ─── Arranque: lanza el primer proceso sin retornar ───────────────────────────
void scheduler_start(void) {
    PCB *first = scheduler_next_ready();
    if (first == NULL) return;

    first->state = PROCESS_RUNNING;
    process_set_current(first);
    scheduler_start_asm(first->rsp);   // definida en interrupts.asm; no retorna
}
