	.equ Mode_USR, 0x10
	.equ Mode_FIQ, 0x11
	.equ Mode_IRQ, 0x12
	.equ Mode_SVC, 0x13
	.equ Mode_ABT, 0x17
	.equ Mode_UND, 0x1B
	.equ Mode_SYS, 0x1F
	.equ Mode_MON, 0x16
	.equ I_Bit,    0x80
	.equ F_Bit,    0x40

	.equ Len_FIQ_Stack,  64
	.equ Len_IRQ_Stack,  64
	.equ Len_ABT_Stack,  64
	.equ Len_UND_Stack,  64
	.equ Len_SVC_Stack,  512
	.equ Len_USR_Stack,  512

	.equ Offset_FIQ_Stack, 0
	.equ Offset_IRQ_Stack, Offset_FIQ_Stack + Len_FIQ_Stack
	.equ Offset_ABT_Stack, Offset_IRQ_Stack + Len_IRQ_Stack
	.equ Offset_UND_Stack, Offset_ABT_Stack + Len_ABT_Stack
	.equ Offset_SVC_Stack, Offset_UND_Stack + Len_UND_Stack
	.equ Offset_USR_Stack, Offset_SVC_Stack + Len_SVC_Stack
	
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
	and	r0, r0, #1					@ Check generic timer support
	beq	1f
	ldr	r0, =24000000				@ 24MHz timer frequency
	mcr	p15, 0, r0, c14, c0, 0		@ CNTFRQ
1:
	@
	@ CPU initialisation
	@
	mrc	p15, 0, r4, c0, c0, 5		@ MPIDR (ARMv7 only)
	and	r4, r4, #15					@ CPU number

2:
	@
	@ UART initialisation (38400 8N1)
	@
	ldr	r0, =0x1c090000				@ UART base (Versatile Express)
	mov	r1, #0x10					@ ibrd
	str	r1, [r0, #0x24]
	mov	r1, #0xc300
	orr	r1, #0x0001					@ cr
	str	r1, [r0, #0x30]

	@ Now we've got rid of the secondary CPUs, set up a stack
	@ for CPU 0 so we can write most of this in C.

	@ set stacktop for monitor mode
	CPS     #Mode_MON
	ldr sp, =mon_stacktop

	@ set stacktop for svc mode
	MSR     CPSR_c, #Mode_SVC | I_Bit | F_Bit /* No interrupts */
	ldr sp, =s_svc_stacktop

	@ And call the C entrypoint
	bl      c_start
	
	@ Never reached
1:	b 1b

	@
	@ Function for C code to make semihosting calls:
	@
	.globl __semi_call
__semi_call:
	svc     0x123456
	mov pc, lr


	.ltorg
	.section .init, "ax"
	.code 32
	
	Mode_MON            =   0x16
	Mode_SVP            =   0x13
	NS_BIT              =   0x1

.global monitor
.align 5
monitor:
	@ Monitor
	NOP     @ Reset      - not used by Monitor
	NOP     @ Undef      - not used by Monitor
	B       SMC_Handler
	NOP     @ Prefetch   - can by used by Monitor
	NOP     @ Data abort - can by used by Monitor
	NOP     @ RESERVED
	NOP     @ IRQ        - can by used by Monitor
	NOP     @ FIQ        - can by used by Monitor


@ ------------------------------------------------------------
@ SMC Handler
@
@ - Detect which world executed SMC
@ - Saves state to appropriate stack
@ - Restores other worlds state
@ - Switches world
@ - Performs exception return
@ ------------------------------------------------------------
.global SMC_Handler
SMC_Handler:
	PUSH   {r0-r3}

	@ Which world have we come from
	@ ------------------------------
	MRC     p15, 0, r0, c1, c1, 0       
	and r0, r0, 0x1
	cmp r0, $NS_BIT
	beq call_secure_world	@ if NS bit is 1, change from normal to secure world
	bne call_normal_world	@ if NS bit is 0, change from secure to normal world

.global call_secure_world
call_secure_world:
	# from normal world
	# change NS first

	MRC     p15, 0, r0, c1, c1, 0
	EOR     r0, r0, #NS_BIT
	MCR     p15, 0, r0, c1, c1, 0   

	@ Load save to pointer
	@ ---------------------
	LDR   r0, =NS_STACK_SP
	LDR     r2, [r0]

	@ Load restore from pointer
	@ --------------------------
	LDR   r1, =S_STACK_SP
	LDR     r3, [r1]

	@ r2  <-- save to
	@ r3  <-- restore from
	
	@ Save general purpose registers, SPSR and LR
	@ --------------------------------------------
	STMFD   r2!, {r4-r12}               @ Save r4 to r12

	cps #Mode_SVC
	stmfd r2!, {r13}
	cps #Mode_MON

	@ ADD SUPPORT FOR SPs
	MRS     r4, spsr                    @ Also get a copy of the SPSR
	STMFD   r2!, {r4, lr}               @ Save original SPSR and LR
	
	STR     r2, [r0]                    @ Save updated pointer back, r0 and r2 now free
	
	@ Restore other world registers, SPSR and LR
	@ ---------------------------------------------
	LDMFD   r3!, {r0, lr}               @ Get SPSR and LR from	

	@ ADD SUPPORT FOR SPs
	MSR     spsr_cxsf, r0               @ Restore SPSR
	
	cps #Mode_SVC
	ldmfd r3!, {r13}
	cps #Mode_MON
	
	LDMFD   r3!, {r4-r12}               @ Restore registers r4 to r12

	STR     r3, [r1]                    @ Save updated pointer back, r1 and r3 now free
	
	@ Clear local monitor
	@ --------------------
	CLREX                               @ Not strictly required in this example, as not using LDR EX/STREX
										@ However, architecturally should execute CLREX on a context switch

	@ Now restore args (r0-r3)
	@ -------------------------
	POP     {r0-r3}
		
	@ Perform exception return
	@ -------------------------
	MOVS    pc, lr
	
.global call_normal_world
call_normal_world:
	# from secure world
	# change NS finally

	@ Load save to pointer
	@ ---------------------
	LDR   r0, =S_STACK_SP             @ If NS bit set, was in Normal world.  So restore Secure state
	LDR     r2, [r0]

	@ Load restore from pointer
	@ --------------------------
	LDR   r1, =NS_STACK_SP
	LDR     r3, [r1]

	@ r2  <-- save to
	@ r3  <-- restore from

	@ Save general purpose registers, SPSR and LR
	@ --------------------------------------------
	STMFD   r2!, {r4-r12}               @ Save r4 to r12

	cps #Mode_SVC
	stmfd r2!, {r13}
	cps #Mode_MON

	@ ADD SUPPORT FOR SPs
	MRS     r4, spsr                    @ Also get a copy of the SPSR
	STMFD   r2!, {r4, lr}               @ Save original SPSR and LR
	
	STR     r2, [r0]                    @ Save updated pointer back, r0 and r2 now free
	
	@ Restore other world registers, SPSR and LR
	@ ---------------------------------------------
	LDMFD   r3!, {r0, lr}               @ Get SPSR and LR from	

	@ ADD SUPPORT FOR SPs
	MSR     spsr_cxsf, r0               @ Restore SPSR
	
	cps #Mode_SVC
	ldmfd r3!, {r13}
	cps #Mode_MON
	
	LDMFD   r3!, {r4-r12}               @ Restore registers r4 to r12

	STR     r3, [r1]                    @ Save updated pointer back, r1 and r3 now free

	@ we change the NS bit finally to backup sp reg in normal svc mode
	MRC     p15, 0, r0, c1, c1, 0
	EOR     r0, r0, #NS_BIT
	MCR     p15, 0, r0, c1, c1, 0   

	@ Clear local monitor
	@ --------------------
	CLREX                               @ Not strictly required in this example, as not using LDR EX/STREX
										@ However, architecturally should execute CLREX on a context switch	

	@ Now restore args (r0-r3)
	@ -------------------------
	POP     {r0-r3}

	@ Perform exception return
	@ -------------------------
	MOVS    pc, lr


@ ------------------------------------------------------------
@ Monitor Initialization
@
@ This is called the first time the Secure world wishes to
@ move to the Normal world.
@ ------------------------------------------------------------

.global monitorInit
monitorInit:
	@ Install Secure Monitor
	@ -----------------------
	LDR r1, =ns_image                    /* R1 is used !!!!*/
	STR r0, [r1]
	LDR r0, =monitor					@ Get address of Monitors vector table
	MCR p15, 0, r0, c12, c0, 1			@ Write Monitor Vector Base Address Register

	@ Save Secure state
	@ ------------------
	LDR     r0, =S_STACK_LIMIT          @ Get address of Secure state stack
	STMFD   r0!, {r4-r12}               @ Save general purpose registers

	ldr r10, =s_svc_stacktop	
	stmfd r0!, {r10}

	@ ADD support for SPs
	MRS     r1, cpsr                    @ Also get a copy of the CPSR

	ldr lr, =Secure_World

	STMFD   r0!, {r1, lr}               @ Save CPSR and LR

	@ Switch to Monitor mode
	@ -----------------------
	CPS     #Mode_MON                   @ Move to Monitor mode after saving Secure state

	@ Save Secure state stack pointer
	@ --------------------------------
	LDR     r1, =S_STACK_SP              @ Get address of global
	STR     r0, [r1]                     @ Save pointer


	@ Set up initial NS state stack pointer
	@ --------------------------------------
	LDR     r0, =NS_STACK_SP             @ Get address of global
	LDR     r1, =NS_STACK_LIMIT          @ Get top of Normal state stack (assuming FD model)
	STR     r1, [r0]                     @ Save pointer


	@ Set up exception return information
	@ ------------------------------------
	@IMPORT  ns_image
	 
	LDR     lr, ns_image				 @ ns_image
	MSR     spsr_cxsf, #Mode_SVP         @ Set SPSR to be SVC mode

	CPS     #Mode_SVC
	ldr r13, =ns_svc_stacktop
	CPS     #Mode_MON

	@ Switch to Normal world
	@ -----------------------
	MRC     p15, 0, r4, c1, c1, 0        @ Read Secure Configuration Register data
	ORR     r4, #NS_BIT                  @ Set NS bit
	MCR     p15, 0, r4, c1, c1, 0        @ Write Secure Configuration Register data
	 
	 
	@ Clear general purpose registers
	@ --------------------------------
	MOV     r0,  #0
	MOV     r1,  #0
	MOV     r2,  #0
	MOV     r3,  #0
	MOV     r4,  #0
	MOV     r5,  #0
	MOV     r6,  #0
	MOV     r7,  #0
	MOV     r8,  #0
	MOV     r9,  #0
	MOV     r10, #0
	MOV     r11, #0
	MOV     r12, #0
	 
	MOVS    pc, lr

@ ------------------------------------------------------------
@ Space reserved for stacks
@ ------------------------------------------------------------
NS_STACK_BASE:
	.word     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
NS_STACK_LIMIT:

S_STACK_BASE:
	.word     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

S_STACK_LIMIT:

NS_STACK_SP:
	.word     0
	
S_STACK_SP:
	.word     0

ns_image :
	.word     0

@.end
@ ------------------------------------------------------------
@ End
@ ------------------------------------------------------------


UndefHandler:
	b UndefHandler

SWIHandler:
	b SWIHandler

PAbortHandler:
	b PAbortHandler

DAbortHandler:
	b DAbortHandler

IRQHandler:
	b IRQHandler

FIQHandler:
	b FIQHandler

# This raise is to handle the 
# compilation error with libgcc.a
.global raise
raise:
	mov pc, lr
