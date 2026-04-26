#include <stdint.h>
#include <stddef.h>
#include "../c/include/userlib.h"
#include "../c/include/shell.h"

static int g_passed = 0;
static int g_failed = 0;

static void print_num(uint64_t n) {
    char buf[24];
    num_to_str(n, buf, 10);
    shellPrintString(buf);
}

static void print_status(MemStatus *s) {
    shellPrintString("  total=");  print_num(s->total);
    shellPrintString(" used=");    print_num(s->used);
    shellPrintString(" free=");    print_num(s->free);
    shellPrintString(" allocs=");  print_num(s->alloc_count);
    shellPrintString("\n");
}

static void check(int cond, const char *msg) {
    if (cond) {
        shellPrintString("  [OK]   ");
        g_passed++;
    } else {
        shellPrintString("  [FAIL] ");
        g_failed++;
    }
    shellPrintString((char *)msg);
    shellPrintString("\n");
}

// Test 1: Alocacion basica, escritura y lectura
static void test_basic(void) {
    shellPrintString("\n-- Test 1: Basico --\n");

    void *p = sys_malloc(128);
    check(p != NULL, "malloc(128) retorna puntero no nulo");

    uint8_t *b = (uint8_t *)p;
    for (int i = 0; i < 128; i++)
        b[i] = (uint8_t)(i ^ 0xA5);

    int ok = 1;
    for (int i = 0; i < 128; i++)
        if (b[i] != (uint8_t)(i ^ 0xA5)) { ok = 0; break; }
    check(ok, "escritura y lectura de 128 bytes correcta");

    sys_free(p);

    MemStatus s;
    sys_mem_status(&s);
    check(s.alloc_count == 0, "alloc_count == 0 tras free");
    check(s.used + s.free == s.total, "used + free == total");
}

// Test 2: Multiples alocaciones sin solapamiento
static void test_multiple(void) {
    shellPrintString("\n-- Test 2: Multiples alocaciones --\n");

#define N       16
#define BSIZE   256
    void *ptrs[N];
    int all_ok = 1;

    for (int i = 0; i < N; i++) {
        ptrs[i] = sys_malloc(BSIZE);
        if (ptrs[i] == NULL) { all_ok = 0; }
    }
    check(all_ok, "16 x malloc(256) exitosos");

    // Escribir patron unico en cada bloque
    for (int i = 0; i < N; i++) {
        if (!ptrs[i]) continue;
        uint8_t *b = (uint8_t *)ptrs[i];
        for (int j = 0; j < BSIZE; j++)
            b[j] = (uint8_t)(i + j);
    }

    // Verificar que ningun bloque pisó a otro
    int no_corruption = 1;
    for (int i = 0; i < N; i++) {
        if (!ptrs[i]) continue;
        uint8_t *b = (uint8_t *)ptrs[i];
        for (int j = 0; j < BSIZE; j++) {
            if (b[j] != (uint8_t)(i + j)) { no_corruption = 0; break; }
        }
    }
    check(no_corruption, "sin corrupcion entre bloques");

    MemStatus s;
    sys_mem_status(&s);
    check(s.alloc_count == N, "alloc_count == 16");
    check(s.used + s.free == s.total, "used + free == total");

    for (int i = 0; i < N; i++) sys_free(ptrs[i]);

    sys_mem_status(&s);
    check(s.alloc_count == 0, "alloc_count == 0 tras liberar todo");
#undef N
#undef BSIZE
}

// Test 3: Coalescencia - liberar en orden inverso
static void test_coalescing(void) {
    shellPrintString("\n-- Test 3: Coalescencia --\n");

#define M 8
    void *ptrs[M];
    MemStatus before, after;
    sys_mem_status(&before);

    for (int i = 0; i < M; i++)
        ptrs[i] = sys_malloc(64);

    // Liberar en orden inverso para forzar coalescencia hacia atras
    for (int i = M - 1; i >= 0; i--)
        sys_free(ptrs[i]);

    sys_mem_status(&after);
    check(after.alloc_count == 0, "alloc_count == 0 tras coalescencia");
    check(after.free >= before.free, "memoria libre recuperada (>= estado inicial)");
    check(after.used + after.free == after.total, "used + free == total");
#undef M
}

// Test 4: Casos limite
static void test_edge(void) {
    shellPrintString("\n-- Test 4: Edge cases --\n");

    void *p = sys_malloc(0);
    check(p == NULL, "malloc(0) retorna NULL");

    // free(NULL) no debe crashear
    sys_free(NULL);
    check(1, "free(NULL) no crashea");

    // Alloc, free, alloc del mismo tamaño: debe retornar puntero valido
    void *p1 = sys_malloc(512);
    sys_free(p1);
    void *p2 = sys_malloc(512);
    check(p2 != NULL, "malloc tras free retorna puntero valido");
    sys_free(p2);

    MemStatus s;
    sys_mem_status(&s);
    check(s.alloc_count == 0, "alloc_count == 0 al final de edge cases");
}

// Test 5: Stress - llenar el heap y vaciarlo
static void test_stress(void) {
    shellPrintString("\n-- Test 5: Stress --\n");

#define SBLOCK (32 * 1024)  // 32 KB por bloque
#define SMAX    300
    static void *ptrs[SMAX];    // static para no saturar el stack
    int count = 0;

    while (count < SMAX) {
        ptrs[count] = sys_malloc(SBLOCK);
        if (ptrs[count] == NULL) break;
        count++;
    }

    shellPrintString("  Bloques de 32 KB alocados: ");
    print_num((uint64_t)count);
    shellPrintString("\n");
    check(count > 0, "al menos un bloque de 32 KB alocado");

    MemStatus s;
    sys_mem_status(&s);
    check(s.used + s.free == s.total, "used + free == total bajo carga maxima");

    for (int i = 0; i < count; i++) sys_free(ptrs[i]);

    sys_mem_status(&s);
    check(s.alloc_count == 0, "heap completamente vaciado");
    check(s.used + s.free == s.total, "used + free == total tras vaciado");
#undef SBLOCK
#undef SMAX
}

void testMM(void) {
    g_passed = 0;
    g_failed = 0;

    shellPrintString("=== Test Memory Manager ===\n");

    MemStatus initial;
    sys_mem_status(&initial);
    shellPrintString("Estado inicial:");
    print_status(&initial);

    test_basic();
    test_multiple();
    test_coalescing();
    test_edge();
    test_stress();

    shellPrintString("\n=== Resultado: ");
    print_num((uint64_t)g_passed);
    shellPrintString(" OK  ");
    print_num((uint64_t)g_failed);
    shellPrintString(" FAIL ===\n");
}
