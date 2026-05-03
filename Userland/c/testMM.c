#include "../include/testMM.h"

static int g_passed = 0;
static int g_failed = 0;

/* Funcion para imprimir numeros. */
void print_num(uint64_t n){
    char buf[BUFF_NUM];
    num_to_str(n, buf, 10);
    shellPrintString(buf);
}

void print_status(MemStatus *s){
    shellPrintString("  total=");  
    print_num(s->total);
    shellPrintString(" used=");    
    print_num(s->used);
    shellPrintString(" free=");    
    print_num(s->free);
    shellPrintString(" allocs=");  
    print_num(s->alloc_count);
    shellPrintString("\n");
}

void check(int cond, const char* msg){
    if(cond){
        shellPrintString("  [OK]   ");
        shellPrintString((char*)msg);
        g_passed++;
    }else{
        shellPrintString("  [FAIL] ");
        shellPrintString((char*)msg);
        g_failed++;

        shellPrintString("// Estado actual tras fallo: ");
        MemStatus s;
        sys_mem_status(&s);
        print_status(&s);

    }

    shellPrintString("\n");
}

/* Test 1: Alocacion basica, escritura y lectura */
void test_basic(void){
    shellPrintString("\n-- Test 1: Basico --\n");

    void* p = sys_malloc(MALLOC_TEST_SIZE);
    check(p != NULL, "malloc(...) retorna puntero no nulo.");

    uint8_t* b = (uint8_t*)p;

    for(int i = 0; i < MALLOC_TEST_SIZE; i++){
        /* Bitwise XOR Pattern para probar la integridad de la memoria. */
        b[i] = (uint8_t)(i ^ 0xA5);
    }

    int ok = 1;

    /* Vuelvo a recorrer el bloque de memoria para verificar la integridad. */
    for(int i = 0; i < MALLOC_TEST_SIZE && ok; i++){
        if(b[i] != (uint8_t)(i ^ 0xA5)){ 
            ok = 0;
        }
    }

    check(ok, "escritura y lectura de bytes correcta.");

    sys_free(p);

    MemStatus s;
    sys_mem_status(&s);

    check(s.alloc_count == 0, "alloc_count == 0 tras free.");
    check(s.used + s.free == s.total, "used + free == total.");
}

/* Test 2: Multiples alocaciones sin solapamiento */
void test_multiple(void){
    shellPrintString("\n-- Test 2: Multiples alocaciones --\n");

    void* ptrs[N];
    int all_ok = 1;

    for(int i = 0; i < N; i++){
        ptrs[i] = sys_malloc(MALLOC_TEST_SIZE * 2);  // Alocar bloques de 256 bytes
        if(ptrs[i] == NULL){
            all_ok = 0; 
        }
    }
    check(all_ok, "16 x malloc(256) exitosos");

    /* Escribir patron unico en cada bloque. */ 
    for(int i = 0; i < N; i++){
        if(!ptrs[i]){
            uint8_t* b = (uint8_t*)ptrs[i];
            
            for(int j = 0; j < (MALLOC_TEST_SIZE * 2); j++){
                b[j] = (uint8_t)(i + j);
            }
        } 
    }

    /* Verificar que ningun bloque pisó a otro */
    int no_corruption = 1;
    for(int i = 0; i < N && no_corruption; i++){

        if(!ptrs[i]){    
            uint8_t* b = (uint8_t *)ptrs[i];

            for(int j = 0; j < (MALLOC_TEST_SIZE * 2) && no_corruption; j++){
                if(b[j] != (uint8_t)(i + j)){
                    no_corruption = 0; 
                }
            }
        } 
    }
    
    check(no_corruption, "sin corrupcion entre bloques.");

    MemStatus s;
    sys_mem_status(&s);
    
    check(s.alloc_count == N, "alloc_count == 16.");
    check(s.used + s.free == s.total, "used + free == total.");

    for(int i = 0; i < N; i++){
        sys_free(ptrs[i]);
    }

    sys_mem_status(&s);

    check(s.alloc_count == 0, "alloc_count == 0 tras liberar todo.");
    check(s.used + s.free == s.total, "used + free == total tras liberar todo.");
}

/* Test 3: Coalescencia - liberar en orden inverso. */
void test_coalescing(void){
    shellPrintString("\n-- Test 3: Coalescencia --\n");

    void* ptrs[M];
    MemStatus before, after;
    sys_mem_status(&before);    /* Guardar estado inicial. */

    for(int i = 0; i < M; i++){
        ptrs[i] = sys_malloc(64);
    }
    
    /* Liberar en orden inverso para forzar coalescencia hacia atras. */ 
    for(int i = M - 1; i >= 0; i--){
        sys_free(ptrs[i]);
    }

    sys_mem_status(&after); /* Guardar estado final. */

    check(after.alloc_count == 0, "alloc_count == 0 tras coalescencia.");
    check(after.free >= before.free, "memoria libre recuperada (correcto).");
    check(after.used + after.free == after.total, "used + free == total.");
}

/* Test 4: Casos limite */
void test_edge(void){
    shellPrintString("\n-- Test 4: Edge cases --\n");

    void* p = sys_malloc(0);
    check(p == NULL, "malloc(0) retorna NULL");

    /* free(NULL) no debe crashear. */ 
    sys_free(NULL);
    check(1, "free(NULL) no crashea.");

    /* Alloc, free, alloc del mismo tamaño: debe retornar puntero valido. */
    /* Chequea correcta allocation y free sin corrupcion respetando el header. */
    void* p1 = sys_malloc(512);
    sys_free(p1);
    void* p2 = sys_malloc(512);
    check(p2 != NULL, "malloc tras free del mismo tamaño retorna puntero valido.");
    sys_free(p2);

    MemStatus s;
    sys_mem_status(&s);
    check(s.alloc_count == 0, "alloc_count == 0 al final de edge cases.");
}

/* Test 5: Stress - llenar el heap y vaciarlo */
void test_stress(void){
    shellPrintString("\n-- Test 5: Stress --\n");

    /* static para no saturar el stack. */
    static void* ptrs[SMAX]; 
    int count = 0;

    while(count < SMAX){
        ptrs[count] = sys_malloc(SBLOCK);

        if(ptrs[count] == NULL){
            break;
        }

        count++;
    }

    shellPrintString("  Bloques de 32 KB alocados: ");
    print_num((uint64_t)count);
    shellPrintString("\n");
    check(count > 0, "al menos un bloque de 32 KB alocados.");

    MemStatus s;
    sys_mem_status(&s);

    check(s.used + s.free == s.total, "used + free == total bajo carga maxima.");

    for(int i = 0; i < count; i++){
        sys_free(ptrs[i]);
    }

    sys_mem_status(&s);

    check(s.alloc_count == 0, "heap completamente vaciado.");
    check(s.used + s.free == s.total, "used + free == total tras vaciado.");
}

void testMM(void){
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
