	.syntax unified
	.arch_extension sec	
	.arch_extension virt
	.text

.global raise
raise:
	mov pc, lr
