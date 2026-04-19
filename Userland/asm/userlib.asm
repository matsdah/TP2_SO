GLOBAL sys_write
GLOBAL sys_read
GLOBAL sys_registers
GLOBAL sys_time
GLOBAL sys_date
GLOBAL sys_clear
GLOBAL sys_increase_fontsize
GLOBAL sys_decrease_fontsize
GLOBAL sys_ticks
GLOBAL sys_beep
GLOBAL sys_speaker_start
GLOBAL sys_speaker_off
GLOBAL sys_screen_width
GLOBAL sys_screen_height
GLOBAL sys_putpixel
GLOBAL sys_fill_rect
GLOBAL gen_invalid_opcode
GLOBAL sys_malloc
GLOBAL sys_free
GLOBAL sys_mem_status

section .text

sys_malloc:
    mov rax, 16
    int 0x80
    ret

sys_free:
    mov rax, 17
    int 0x80
    ret

sys_mem_status:
    mov rax, 18
    int 0x80
    ret

sys_registers:
	mov rax, 0
	int 0x80
	ret

sys_time:
	mov rax, 1
	int 0x80
	ret

sys_date:
	mov rax, 2
	int 0x80
	ret

sys_read:
	mov rax, 3
	int 0x80
	ret

sys_write:
	mov rax, 4
	int 0x80
	ret

sys_increase_fontsize:
    	mov rax, 5
	int 0x80
	ret

sys_decrease_fontsize:
   	mov rax, 6
	int 0x80
	ret

sys_beep:
    	mov rax, 7
	int 0x80
	ret

sys_ticks:
    mov rax, 8
    int 0x80
    ret

sys_clear:
    mov rax, 9
	int 0x80
	ret

sys_speaker_start:
	mov rax, 10
	int 0x80
	ret

sys_speaker_off:
	mov rax, 11
	int 0x80
	ret

sys_screen_width:
	mov rax, 12
	int 0x80
	ret

sys_screen_height:
	mov rax, 13
	int 0x80
	ret

sys_putpixel:
	mov rax, 14
	int 0x80
	ret

sys_fill_rect:
	mov rax, 15
	int 0x80
	ret

gen_invalid_opcode:
    ud2
    ret