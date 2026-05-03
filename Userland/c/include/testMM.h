#include <stdint.h>
#include <stddef.h>
#include "../../c/include/userlib.h"
#include "../../c/include/shell.h"

#define BUFF_NUM 32
#define MALLOC_TEST_SIZE 128
#define N 16
#define M 8
#define SBLOCK (32 * 1024)  // 32 KB por bloque
#define SMAX 300

void print_num(uint64_t n);

void print_status(MemStatus *s);

void check(int cond, const char* msg);

/* Test 1: Alocacion basica, escritura y lectura */
void test_basic();

/* Test 2: Multiples alocaciones sin solapamiento */
void test_multiple();

/* Test 3: Coalescencia - liberar en orden inverso. */
void test_coalescing();

/* Test 4: Edge cases - free(NULL), malloc(0), malloc+free+malloc. */
void test_edge();

/* Test 5: Stress - llenar el heap y vaciarlo. */
void test_stress();

void testMM();