#ifndef SYSCALLDISPATCHER_H
#define SYSCALLDISPATCHER_H

#include <stdint.h>
#include "defs.h"

extern void * syscalls[CANT_SYS];

uint64_t sys_write(uint64_t fd, const char * buff, uint64_t count);
uint64_t sys_read(char * buff, uint64_t count);
uint64_t sys_registers(char * buff);
void sys_time(uint8_t * buff);
void sys_date(uint8_t * buff);
void sys_decrease_fontsize(void);
void sys_increase_fontsize(void);
void sys_beep(uint32_t freq, uint64_t time);
uint64_t sys_ticks(void);
void sys_clear(void);
uint64_t sys_screen_width(void);
uint64_t sys_screen_height(void);
void sys_putpixel(uint32_t color, uint64_t x, uint64_t y);
void sys_fill_rect(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint32_t color);
void sys_speaker_start(uint32_t freq);
void sys_speaker_off(void);
uint64_t sys_malloc(uint64_t size);
void sys_free(uint64_t ptr);
void sys_mem_status(uint64_t statusPtr);

#endif