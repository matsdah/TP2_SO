#include <stdint.h>
#include "lib.h"
#include "moduleLoader.h"
#include "idtLoader.h"
#include "kernelApi.h"
#include "memoryManager.h"

#define HEAP_START  0x600000
#define HEAP_SIZE   (8 * 1024 * 1024)  // 8 MB

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;
static const uint64_t PageSize = 0x1000;
static void * const sampleCodeModuleAddress = (void*)0x400000;
static void * const sampleDataModuleAddress = (void*)0x500000;

typedef int (*EntryPoint)(void);

// Punto de entrada del kernel: transfiere control al módulo de usuario
int main(void){
	((EntryPoint)sampleCodeModuleAddress)();
	return 0;
}

// Limpia la sección BSS del kernel
void clearBSS(void * bssAddress, uint64_t bssSize){
	memset(bssAddress, 0, bssSize);
}

// Calcula la base de la pila del kernel
void * getStackBase(void){
	return (void*)(
		(uint64_t)&endOfKernel
		+ PageSize * 8	
		- sizeof(uint64_t)
	);
}

// Carga módulos, limpia BSS e inicializa la IDT
void * initializeKernelBinary(void){

	void * moduleAddresses[] = {sampleCodeModuleAddress, sampleDataModuleAddress};

	loadModules(&endOfKernelBinary, moduleAddresses);
	clearBSS(&bss, &endOfKernel - &bss);
	load_idt();

	// Inicilizacion del administrador de memoria
	mm_init((void *)HEAP_START, HEAP_SIZE);
	
	return getStackBase();
}

