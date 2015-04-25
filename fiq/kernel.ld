/*
 * model.lds.S - simple linker script for stand-alone Linux booting
 *
 * Copyright (C) 2011 ARM Limited. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE.txt file.
 */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
TARGET(binary)

ENTRY_SVC_STACK_SIZE = 0x1000;
ENTRY_IRQ_STACK_SIZE = 0x1000;
ENTRY_FIQ_STACK_SIZE = 0x1000;

SECTIONS
{
	. = 0x80010000;
 .text : { *(.text) }
 .data : { *(.data) }
 .bss : { *(.bss) }

 /*define a stack for the entry*/
	. = ALIGN(0x1000);
	. += ENTRY_SVC_STACK_SIZE;
	svc_stacktop = .;	
	
	. = ALIGN(0x1000);
	. += ENTRY_IRQ_STACK_SIZE;
	irq_stacktop = .;
	
	. = ALIGN(0x1000);
	. += ENTRY_FIQ_STACK_SIZE;
	fiq_stacktop = .;

}
