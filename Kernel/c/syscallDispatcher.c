#include "syscallDispatcher.h"
#include "videoDriver.h"
#include "keyboardDriver.h"
#include "lib.h"
#include "defs.h"
#include "time.h"
#include "sound.h"
#include "memoryManager.h"

// Syscall 16: malloc
uint64_t sys_malloc(uint64_t size) {
    return (uint64_t)mm_malloc(size);
}

// Syscall 17: free
void sys_free(uint64_t ptr) {
    mm_free((void *)ptr);
}

// Syscall 18: mem status
void sys_mem_status(uint64_t statusPtr) {
    mm_status((MemStatus *)statusPtr);
}

// Aumenta el tamaño de fuente por defecto
void sys_increase_fontsize(){
    increaseFontSize();
}
void sys_decrease_fontsize(){
    decreaseFontSize();
}

// Limpia la pantalla en modo gráfico (negro)
void sys_clear(){
    clearScreen(0x000000);
}

// Escribe 'count' bytes en la salida indicada (texto gráfico)
uint64_t sys_write(uint64_t fd, const char * buff, uint64_t count){
    (void)fd;
    uint32_t color = 0xFFFFFF;

    for(uint64_t i = 0; i < count; i++){
        videoPutChar((uint8_t)buff[i], color);
    }

    return count;
}

uint64_t sys_read(char * buff, uint64_t count){
   return readKeyBuff(buff, count);
}

void sys_date(uint8_t * buff){
    date(buff);
}

void sys_time(uint8_t * buff){
    time(buff);
}

uint64_t sys_registers(char * buff){
    return copyRegistersBuffer(buff);
}

void sys_beep(uint32_t freq, uint64_t time){
    beep(freq, time);
}

// Retorna el contador de ticks desde el arranque
uint64_t sys_ticks(){
    return deltaTicks();
}

void sys_speaker_start(uint32_t freq){
    startSpeaker(freq);
}

void sys_speaker_off(){
    turnOff();
}

uint64_t sys_screen_width(){
    return (uint64_t)getScreenWidth();
}

uint64_t sys_screen_height(){
    return (uint64_t)getScreenHeight();
}

void sys_putpixel(uint32_t color, uint64_t x, uint64_t y){
    putPixel(color, x, y);
}

void sys_fill_rect(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint32_t color){
    fillRect(x, y, w, h, color);
}

void * syscalls[CANT_SYS] = {
    &sys_registers,         // 0
    &sys_time,              // 1
    &sys_date,              // 2
    &sys_read,              // 3
    &sys_write,             // 4
    &sys_increase_fontsize, // 5
    &sys_decrease_fontsize, // 6
    &sys_beep,              // 7
    &sys_ticks,             // 8
    &sys_clear,             // 9
    &sys_speaker_start,     // 10
    &sys_speaker_off,       // 11
    &sys_screen_width,      // 12
    &sys_screen_height,     // 13
    &sys_putpixel,          // 14
    &sys_fill_rect,         // 15
    &sys_malloc,            // 16
    &sys_free,              // 17
    &sys_mem_status         // 18
};