.equ Mode_USR, 0x10
.equ Mode_FIQ, 0x11
.equ Mode_IRQ, 0x12
.equ Mode_SVC, 0x13
.equ Mode_ABT, 0x17
.equ Mode_UND, 0x1B
.equ Mode_SYS, 0x1F
.equ Mode_MON, 0x16
.equ MODE_MASK, 0x1f


	.syntax	unified
	.arch_extension sec
	.arch_extension virt
	.text

	.globl	start
start:
	b reset
	ldr pc, UndefAddr
	ldr pc, SWIAddr
	ldr pc, PAbortAddr
	ldr pc, DAbortAddr
	ldr pc, ReservedAddr
	ldr pc, IRQAddr
	ldr pc, FIQAddr

UndefAddr:      .word UndefHandler
SWIAddr:        .word SWIHandler
PAbortAddr:     .word PAbortHandler
DAbortAddr:     .word DAbortHandler
ReservedAddr:   .word 0
IRQAddr:        .word IRQHandler
FIQAddr:        .word FIQHandler

reset:
	@
	@ Program architected timer frequency
	@
	mrc	p15, 0, r0, c0, c1, 1		@ CPUID_EXT_PFR1
	lsr	r0, r0, #16
	and	r0, r0, #1			@ Check generic timer support
	beq	1f
	ldr	r0, =24000000			@ 24MHz timer frequency
	mcr	p15, 0, r0, c14, c0, 0		@ CNTFRQ
1:
	@
	@ CPU initialisation
	@
	mrc	p15, 0, r4, c0, c0, 5		@ MPIDR (ARMv7 only)
	and	r4, r4, #15			@ CPU number

2:
	@
	@ UART initialisation (38400 8N1)
	@
	ldr	r0, =0x1c090000			@ UART base (Versatile Express)
	mov	r1, #0x10			@ ibrd
	str	r1, [r0, #0x24]
	mov	r1, #0xc300
	orr	r1, #0x0001			@ cr
	str	r1, [r0, #0x30]

	@ Now we've got rid of the secondary CPUs, set up a stack
	@ for CPU 0 so we can write most of this in C.
	ldr sp, =svc_stacktop

	cps #Mode_IRQ
	ldr sp, =irq_stacktop
	cps #Mode_SVC
	
	cps #Mode_FIQ
	ldr sp, =fiq_stacktop
	cps #Mode_FIQ

	@Write Secure World Vector Base Address
	@ ---------------------------------------
	LDR r0, =start
	MCR p15, 0, r0, c12, c0, 0

	@ And call the C entrypoint
	bl      c_start
	@ Never reached
1:	b 1b


@------------------------------------------------------------
@ Vector handler
@ -----------------------------------------------------------
UndefHandler:
	b UndefHandler

SWIHandler:
	push {lr}
	bl swi_handler
	pop {lr}
	mov pc, lr

PAbortHandler:
	b PAbortHandler

DAbortHandler:
	b DAbortHandler

IRQHandler:
	# Now in IRQ mode
	sub r14, r14, #4
	stmfd r13!, {r0}
	stmfd r13!, {r1-r2, r14}
	mov r0, r13
	add r13, r13, #16

	# Switch from IRQ mode to SVC mode
	MRS     r2, cpsr
	BIC     r2, r2, #MODE_MASK
	ORR     r2, r2, #Mode_SVC
	MSR     cpsr_cxsf, r2

	push {r3-r12}
	ldmfd r0!, {r1-r2, r14}
	push {r1-r2, r14}
	mov r1, r0
	ldmfd r1!, {r0}
	push {r0}

	bl irq_handler

	pop {r0}
	pop {r1-r2, r14}
	pop {r3-r12}
	mov pc, lr

FIQHandler:
	SUB     r14, r14, #4            // lr: return address after the fiq handler
	STMFD   r13!, {r0-r12, r14}
	MRS     r2, spsr                // copy spsr to r2
	STMFD   r13!, {r2}              // save r2(spsr) to the stack
	STMFD   r13!, {r14}             // save r14 again (it is not really correct)
	STMFD   r13, {sp, lr}^          // save user mode sp and lr
	SUB     r13, r13, #8

	# call traps (trapframe *fp)
	MOV     r0, r13                 // save trapframe as the first parameter
	BL      fiq_handler
	B . 

.global raise
raise:
	mov pc, lr	
