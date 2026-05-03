#include "process.h"
#include "scheduler.h"
#include "memoryManager.h"
#include "lib.h"
#include <stddef.h>

static PCB      process_table[MAX_PROCESSES];
static uint64_t next_pid = 0;
static PCB     *current_process = NULL;

// ─── helpers ─────────────────────────────────────────────────────────────────

static int str_len(const char *s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}

static void str_copy(char *dst, const char *src, int max) {
    int i = 0;
    while (i < max - 1 && src && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

// ─── stack frame inicial ──────────────────────────────────────────────────────
//
// Layout del stack tras una interrupcion en este kernel (ring 0):
//
//   [rsp+17*8] RFLAGS          <- empujado por CPU (parte alta del frame)
//   [rsp+16*8] CS
//   [rsp+15*8] RIP
//   [rsp+14*8] RAX  (primero en pushState)
//   [rsp+13*8] RBX
//   ...
//   [rsp+ 0*8] R15  (ultimo en pushState, primer pop)
//
// Para que popState+iretq arranquen un proceso nuevo, construimos exactamente
// este layout desde la cima del stack hacia abajo.

static uint64_t *build_initial_stack(void *stack_top,
                                     ProcessEntry entry,
                                     int argc, char **argv) {
    uint64_t *sp = (uint64_t *)stack_top;

    // Hardware frame (iretq consume estos 3 valores)
    *(--sp) = 0x202;               // RFLAGS: IF=1
    *(--sp) = 0x08;                // CS: segmento de codigo del kernel
    *(--sp) = (uint64_t)entry;     // RIP: punto de entrada

    // Registros GP en orden de pushState (rax primero → r15 ultimo)
    *(--sp) = 0;                   // RAX
    *(--sp) = 0;                   // RBX
    *(--sp) = 0;                   // RCX
    *(--sp) = 0;                   // RDX
    *(--sp) = 0;                   // RBP
    *(--sp) = (uint64_t)argc;      // RDI → arg1 de la funcion de entrada
    *(--sp) = (uint64_t)argv;      // RSI → arg2 de la funcion de entrada
    *(--sp) = 0;                   // R8
    *(--sp) = 0;                   // R9
    *(--sp) = 0;                   // R10
    *(--sp) = 0;                   // R11
    *(--sp) = 0;                   // R12
    *(--sp) = 0;                   // R13
    *(--sp) = 0;                   // R14
    *(--sp) = 0;                   // R15 (ultimo en pushState → primer pop)

    return sp;   // PCB.rsp apunta aqui
}

// ─── API publica ──────────────────────────────────────────────────────────────

void process_init(void) {
    memset(process_table, 0, sizeof(process_table));
    next_pid = 0;
    current_process = NULL;
}

PCB *process_current(void) {
    return current_process;
}

void process_set_current(PCB *p) {
    current_process = p;
}

PCB *process_get(uint64_t pid) {
    if (pid == 0) return NULL;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (process_table[i].pid == pid &&
            process_table[i].state != PROCESS_FREE)
            return &process_table[i];
    return NULL;
}

int process_create(const char *name, ProcessEntry entry, int argc, char **argv, uint8_t fg) {
    // Buscar slot libre
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_FREE) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;

    PCB *p = &process_table[slot];

    // Asignar stack (kernel-internal: no se contabiliza en alloc_count del usuario)
    p->stack_base = (uint64_t *)mm_malloc_kernel(STACK_SIZE);
    if (p->stack_base == NULL) return -1;

    // Construir frame inicial
    void *stack_top = (void *)((uint8_t *)p->stack_base + STACK_SIZE);
    p->rsp = build_initial_stack(stack_top, entry, argc, argv);

    // Rellenar PCB
    p->pid              = ++next_pid;
    p->priority         = DEFAULT_PRIORITY;
    p->remaining_quanta = DEFAULT_PRIORITY;
    p->state            = PROCESS_READY;
    p->foreground       = fg;
    p->fd[0]            = 0;
    p->fd[1]            = 1;
    p->parent_pid       = (current_process != NULL) ? current_process->pid : 0;
    p->waiting_for      = 0;
    p->argc             = argc;
    p->argv             = argv;
    p->retval           = 0;

    str_copy(p->name, name, MAX_NAME_LEN);
    if (str_len(p->name) == 0) {
        p->name[0] = '?';
        p->name[1] = '\0';
    }

    scheduler_add(p);
    return (int)p->pid;
}

void process_exit(int retval) {
    if (current_process == NULL) return;

    current_process->retval = retval;
    current_process->state  = PROCESS_ZOMBIE;

    // Despertar al padre si esta bloqueado en waitpid esperando este proceso
    PCB *parent = process_get(current_process->parent_pid);
    if (parent != NULL &&
        parent->state == PROCESS_BLOCKED &&
        parent->waiting_for == current_process->pid) {

        // Escribir el retval directamente en el RAX guardado del padre.
        // parent->rsp apunta al slot R15; el slot RAX esta 14 qwords mas arriba.
        parent->rsp[14] = (uint64_t)(int64_t)retval;
        parent->waiting_for = 0;
        parent->state = PROCESS_READY;
    }

    // Liberar stack del proceso que muere
    mm_free(current_process->stack_base);
    current_process->stack_base = NULL;
    current_process->rsp        = NULL;

    force_switch = 1;   // ceder CPU en el proximo retorno de syscall
}

void process_kill(uint64_t pid) {
    PCB *p = process_get(pid);
    if (p == NULL || p->state == PROCESS_FREE || p->state == PROCESS_ZOMBIE)
        return;

    p->retval = -1;

    // Despertar padre si estaba esperando
    PCB *parent = process_get(p->parent_pid);
    if (parent != NULL &&
        parent->state == PROCESS_BLOCKED &&
        parent->waiting_for == pid) {
        parent->rsp[14]     = (uint64_t)(uint32_t)(-1);
        parent->waiting_for = 0;
        parent->state       = PROCESS_READY;
    }

    if (p == current_process) {
        // Suicidio: liberar stack y ceder CPU
        current_process->state = PROCESS_ZOMBIE;
        mm_free(current_process->stack_base);
        current_process->stack_base = NULL;
        current_process->rsp        = NULL;
        force_switch = 1;
    } else {
        // Matar otro proceso: liberar recursos inmediatamente
        scheduler_remove(p);
        if (p->stack_base != NULL) {
            mm_free(p->stack_base);
            p->stack_base = NULL;
        }
        p->state = PROCESS_FREE;
        p->pid   = 0;
    }
}

void process_block(uint64_t pid) {
    PCB *p = process_get(pid);
    if (p == NULL || p->state == PROCESS_FREE || p->state == PROCESS_ZOMBIE)
        return;
    if (p->state == PROCESS_BLOCKED) return;
    p->state = PROCESS_BLOCKED;
    if (p == current_process)
        force_switch = 1;
}

void process_unblock(uint64_t pid) {
    PCB *p = process_get(pid);
    if (p == NULL || p->state != PROCESS_BLOCKED) return;
    p->state = PROCESS_READY;
}

void process_nice(uint64_t pid, uint8_t new_priority) {
    PCB *p = process_get(pid);
    if (p == NULL) return;
    if (new_priority < MIN_PRIORITY) new_priority = MIN_PRIORITY;
    if (new_priority > MAX_PRIORITY) new_priority = MAX_PRIORITY;
    p->priority = new_priority;
    if (p->remaining_quanta > new_priority)
        p->remaining_quanta = new_priority;
}

int process_waitpid(uint64_t pid) {
    PCB *child = process_get(pid);
    if (child == NULL) return -1;

    if (child->state == PROCESS_ZOMBIE) {
        int retval = child->retval;
        // Reap: liberar slot
        child->state = PROCESS_FREE;
        child->pid   = 0;
        return retval;
    }

    // Hijo todavia vivo: bloquear y esperar
    if (current_process != NULL) {
        current_process->state      = PROCESS_BLOCKED;
        current_process->waiting_for = pid;
    }
    force_switch = 1;
    // El retval real sera escrito por process_exit en rsp[14].
    // Devolvemos 0 por ahora; se sobreescribira antes de que userland lo vea.
    return 0;
}

uint64_t process_ps(ProcessInfo *buf, uint64_t max) {
    uint64_t count = 0;
    for (int i = 0; i < MAX_PROCESSES && count < max; i++) {
        PCB *p = &process_table[i];
        if (p->state == PROCESS_FREE) continue;
        buf[count].pid        = p->pid;
        buf[count].priority   = p->priority;
        buf[count].state      = (uint8_t)p->state;
        buf[count].foreground = p->foreground;
        buf[count].rsp        = (uint64_t)p->rsp;
        str_copy(buf[count].name, p->name, MAX_NAME_LEN);
        count++;
    }
    return count;
}
