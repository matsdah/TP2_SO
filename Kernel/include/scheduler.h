#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "process.h"

// Definido en scheduler.c, seteado a 1 para solicitar un context switch voluntario.
// Leido desde interrupts.asm en _irq128Handler y _irq00Handler.
extern volatile uint64_t force_switch;

void scheduler_init(void);
void scheduler_add(PCB *p);
void scheduler_remove(PCB *p);
PCB *scheduler_next_ready(void);
uint64_t *scheduler_tick(uint64_t *current_rsp);
uint64_t *scheduler_yield_impl(uint64_t *current_rsp);
void scheduler_start(void);

// Definida en interrupts.asm: carga rsp del primer proceso y hace popState+iretq
void scheduler_start_asm(uint64_t *rsp);

#endif
