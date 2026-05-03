#include "scheduler.h"
#include "process.h"
#include "time.h"

// Leida desde interrupts.asm para decidir si hacer context switch voluntario
volatile uint64_t force_switch = 0;

/* Lista de procesos listos para ejecutarse */
static PCB* run_queue[MAX_PROCESSES];      

static int queue_size = 0;
static int queue_idx  = 0;

/* Inicializa la cola de ejecución. */
void scheduler_init(void){
    queue_size = 0;         /* Tamaño de la cola. */
    queue_idx = 0;         /* Índice del proceso actual en la cola (round-robin). */
}

/* Agrega un proceso a la cola de ejecución. */
void scheduler_add(PCB* p){
    if(queue_size < MAX_PROCESSES){
        run_queue[queue_size++] = p;
    }
}

/* Elimina un proceso de la cola de ejecución. */
void scheduler_remove(PCB* p){
    for(int i = 0; i < queue_size; i++){

        if(p == run_queue[i]){

            /* Reemplazar con el último proceso. */
            run_queue[i] = run_queue[--queue_size]; 

            if((queue_idx >= queue_size) && (queue_size > 0)){
                /* Caso en el que estaba procesando el último proceso. */
                queue_idx = queue_size - 1;
            }

            return;
        }
    }
}

/* Elegir el siguiente proceso READY usando round-robin */
PCB* scheduler_next_ready(void){
    if(queue_size == 0){
        return NULL;
    }

    for(int i = 0; i < queue_size; i++){

        /* Avanzo de forma circular. */
        queue_idx = (queue_idx + 1) % queue_size;
        
        /* Selecciono el proceso actual. */
        PCB* c = run_queue[queue_idx];
        
        if(c->state == PROCESS_READY || c->state == PROCESS_RUNNING){   // Genera error, a veces muestra 2 procesos como RUNNING
            return c;
        }
    }

    /* Si no hay ninguno READY, devolver el primero de la cola (idle) */
    return run_queue[0];
}

// ─── Llamada desde _irq00Handler (timer) ─────────────────────────────────────
uint64_t* scheduler_tick(uint64_t *current_rsp){

    /* actualizar contador de ticks. */ 
    timer_handler(); 

    PCB* cur = process_current();

    if(cur != NULL){
        cur->rsp = current_rsp;

        if(cur->state == PROCESS_RUNNING){
            if(cur->remaining_quanta > 0){
                /* Decrementar el tiempo restante por c/ tick. */
                cur->remaining_quanta--;
            }

            if(cur->remaining_quanta == 0){
                /* Si se agotó el tiempo asignado, marcar como READY directamente.*/
                cur->state = PROCESS_READY;
                cur->remaining_quanta = cur->priority;
            }
        }
    }

    /* Logica de selección del siguiente proceso. */

    PCB* next = scheduler_next_ready();

    if(next == NULL){
        /* No hay procesos: devolver el mismo RSP (no cambiar nada). */ 
        return current_rsp;
    }
    next->state = PROCESS_RUNNING;
    process_set_current(next);

    return next->rsp;
}

/* Implementación de yield (pausa). Llamada desde _irq128Handler cuando force_switch = 1. */
uint64_t *scheduler_yield_impl(uint64_t *current_rsp){
    PCB* cur = process_current();

    if(cur != NULL){
        cur->rsp = current_rsp;

        if(cur->state == PROCESS_RUNNING){
            /* Lo pauso. */
            cur->state = PROCESS_READY;
        }
    }

    PCB* next = scheduler_next_ready();
    if(next == NULL){
        return current_rsp;
    }

    next->state = PROCESS_RUNNING;
    process_set_current(next);

    return next->rsp;
}

// ─── Arranque: lanza el primer proceso sin retornar ───────────────────────────
void scheduler_start(void){
    PCB* first = scheduler_next_ready();
    if(first == NULL){
        return;
    }

    first->state = PROCESS_RUNNING;
    process_set_current(first);

    /* Definida en interrupts.asm; no retorna */
    scheduler_start_asm(first->rsp);   
}
