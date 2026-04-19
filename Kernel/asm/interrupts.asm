GLOBAL _cli
GLOBAL _sti
GLOBAL picMasterMask
GLOBAL picSlaveMask
GLOBAL haltcpu
GLOBAL _hlt
GLOBAL load_idt_asm
GLOBAL _irq00Handler
GLOBAL _irq01Handler
GLOBAL _irq02Handler
GLOBAL _irq03Handler
GLOBAL _irq04Handler
GLOBAL _irq05Handler
GLOBAL _irq128Handler
GLOBAL _exception0Handler
GLOBAL _exception6Handler
GLOBAL syscallIntRoutine
GLOBAL pressed_key
GLOBAL regsArray
GLOBAL kbd_scancode_read
EXTERN irqDispatcher
EXTERN int80Dispatcher
EXTERN exceptionDispatcher
EXTERN getStackBase
EXTERN syscalls

SECTION .text

%macro pushState 0
	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro popState 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
	pop rax
%endmacro

%macro irqHandlerMaster 1
	pushState

	mov rdi, %1 ; pasaje de parametro
	call irqDispatcher

	mov al, 20h
	out 20h, al

	popState
	iretq
%endmacro

%macro exceptionHandler 1
	pushState

	; Guardar snapshot de registros en una excepción
	; Layout tras pushState: [r15..rax] luego frame de hardware (RIP,CS,RFLAGS,RSP,SS)
	mov     rax, [rsp + 14*8]   ; RAX original
	mov     [regsArray + 0*8], rax
	mov     rax, [rsp + 13*8]   ; RBX
	mov     [regsArray + 1*8], rax
	mov     rax, [rsp + 12*8]   ; RCX
	mov     [regsArray + 2*8], rax
	mov     rax, [rsp + 11*8]   ; RDX
	mov     [regsArray + 3*8], rax
	mov     rax, [rsp + 10*8]   ; RBP
	mov     [regsArray + 4*8], rax
	mov     rax, [rsp + 9*8]    ; RDI
	mov     [regsArray + 5*8], rax
	mov     rax, [rsp + 8*8]    ; RSI
	mov     [regsArray + 6*8], rax
	mov     rax, [rsp + 7*8]    ; R8
	mov     [regsArray + 7*8], rax
	mov     rax, [rsp + 6*8]    ; R9
	mov     [regsArray + 8*8], rax
	mov     rax, [rsp + 5*8]    ; R10
	mov     [regsArray + 9*8], rax
	mov     rax, [rsp + 4*8]    ; R11
	mov     [regsArray + 10*8], rax
	mov     rax, [rsp + 3*8]    ; R12
	mov     [regsArray + 11*8], rax
	mov     rax, [rsp + 2*8]    ; R13
	mov     [regsArray + 12*8], rax
	mov     rax, [rsp + 1*8]    ; R14
	mov     [regsArray + 13*8], rax
	mov     rax, [rsp + 0*8]    ; R15
	mov     [regsArray + 14*8], rax

	; Frame de hardware
	mov     rax, [rsp + 15*8]   ; RIP
	mov     [regsArray + 15*8], rax
	mov     rax, [rsp + 16*8]   ; CS
	mov     [regsArray + 16*8], rax
	mov     rax, [rsp + 17*8]   ; RFLAGS
	mov     [regsArray + 17*8], rax
	mov     rax, [rsp + 18*8]   ; RSP (si aplica)
	mov     [regsArray + 18*8], rax
	mov     rax, [rsp + 19*8]   ; SS (si aplica)
	mov     [regsArray + 19*8], rax
	mov rdi, %1 
	mov rsi, rsp
	call exceptionDispatcher

popState
	call getStackBase	

	mov qword [rsp+8*3], rax				
	mov qword [rsp], userland	
	iretq
%endmacro

_hlt:
	sti
	hlt
	ret

_cli:
	cli
	ret

_sti:
	sti
	ret

; C-callable wrapper to load IDT without using inline asm in C
; void load_idt_asm(void *idtr);
; SysV AMD64: first arg in RDI -> pointer to IDTR descriptor (10 bytes)
load_idt_asm:
	lidt    [rdi]
	ret

picMasterMask:
	push    rbp
   	mov     rbp, rsp
    	mov     ax, di
    	out	   21h,al
    	pop     rbp
    	retn

picSlaveMask:
	push    rbp
    	mov     rbp, rsp
    	mov     ax, di 
    	out	   0A1h, al
    	pop     rbp
    	retn

;8254 Timer (Timer Tick)
_irq00Handler:
	irqHandlerMaster 0

;Keyboard
; IRQ1 - Teclado: captura scancode y opcionalmente snapshot de registros.
_irq01Handler:
	; Guardar contexto completo al entrar (para snapshot correcto)
	pushState

	; Leer scancode y exponerlo a C
	xor     eax, eax
	in      al, 0x60
	mov     [pressed_key], rax

	; Si es la tecla de snapshot (CTRL), copiar contexto salvado
	cmp     al, SNAPSHOT_KEY
	jne     .no_snapshot

	; Layout tras pushState (desde [rsp]):
	;  r15, r14, ..., rax (15 regs) | RIP, CS, RFLAGS, RSP, SS
	mov     rax, [rsp + 14*8]   ; RAX original
	mov     [regsArray + 0*8], rax
	mov     rax, [rsp + 13*8]   ; RBX
	mov     [regsArray + 1*8], rax
	mov     rax, [rsp + 12*8]   ; RCX
	mov     [regsArray + 2*8], rax
	mov     rax, [rsp + 11*8]   ; RDX
	mov     [regsArray + 3*8], rax
	mov     rax, [rsp + 10*8]   ; RBP
	mov     [regsArray + 4*8], rax
	mov     rax, [rsp + 9*8]    ; RDI
	mov     [regsArray + 5*8], rax
	mov     rax, [rsp + 8*8]    ; RSI
	mov     [regsArray + 6*8], rax
	mov     rax, [rsp + 7*8]    ; R8
	mov     [regsArray + 7*8], rax
	mov     rax, [rsp + 6*8]    ; R9
	mov     [regsArray + 8*8], rax
	mov     rax, [rsp + 5*8]    ; R10
	mov     [regsArray + 9*8], rax
	mov     rax, [rsp + 4*8]    ; R11
	mov     [regsArray + 10*8], rax
	mov     rax, [rsp + 3*8]    ; R12
	mov     [regsArray + 11*8], rax
	mov     rax, [rsp + 2*8]    ; R13
	mov     [regsArray + 12*8], rax
	mov     rax, [rsp + 1*8]    ; R14
	mov     [regsArray + 13*8], rax
	mov     rax, [rsp + 0*8]    ; R15
	mov     [regsArray + 14*8], rax

	; Frame de hardware
	mov     rax, [rsp + 15*8]   ; RIP
	mov     [regsArray + 15*8], rax
	mov     rax, [rsp + 16*8]   ; CS
	mov     [regsArray + 16*8], rax
	mov     rax, [rsp + 17*8]   ; RFLAGS
	mov     [regsArray + 17*8], rax
	mov     rax, [rsp + 18*8]   ; RSP
	mov     [regsArray + 18*8], rax
	mov     rax, [rsp + 19*8]   ; SS
	mov     [regsArray + 19*8], rax

.no_snapshot:
	; Despachar IRQ teclado
	mov     rdi, 1
	call    irqDispatcher
	mov     al, 20h
	out     20h, al
	popState
	iretq

;Cascade pic never called
_irq02Handler:
	irqHandlerMaster 2

;Serial Port 2 and 4
_irq03Handler:
	irqHandlerMaster 3

;Serial Port 1 and 3
_irq04Handler:
	irqHandlerMaster 4

;USB
_irq05Handler:
	irqHandlerMaster 5

;Zero Division Exception
_exception0Handler:
	exceptionHandler 0

;Invalid Opcode Exception
_exception6Handler:
	exceptionHandler 6

_irq128Handler:
	pushState
	; validar índice de syscall (usar el mismo valor que CANT_SYS en defs.h)
	cmp rax, 19
	jge .syscall_end
	call [syscalls + rax * 8]

.syscall_end:
    mov [aux], rax
    popState
    mov rax, [aux]
    iretq

haltcpu:
	cli
	hlt
	ret

; Lee el scancode almacenado por la ISR y lo limpia (sin requerir calificador especial en C)
kbd_scancode_read:
	push rbp
	mov rbp, rsp
	xor eax, eax
	mov al, [pressed_key]
	mov byte [pressed_key], 0
	pop rbp
	ret

SECTION .bss
	aux resq 1
	; # de registros
	regsArray resq 20 
	pressed_key resq 1
	SNAPSHOT_KEY equ 0x1D

SECTION .data 
	userland equ 0x400000 