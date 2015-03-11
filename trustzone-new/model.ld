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

PHYS_OFFSET = 0x80000000;
MON_OFFSET  = 0xf0000000;
STACKTOP = 0xf2000000;

S_SVC_STACKTOP  = 0xf3000000;
MON_STACKSTOP   = 0xf5000000;
NS_SVC_STACKTOP = 0xf8000000;

SECTIONS
{
 . = PHYS_OFFSET;

 . = PHYS_OFFSET + 0x8000 - 0x40;

 kernel_start = .;
 kernel_entry = . + 0x40;
 kernel_end = .;

 . = PHYS_OFFSET + 0x00d00000;
 fs_start = .;
 fs_end = .;


 . = MON_OFFSET;
 /* Put most of the actual boot loader code up in high memory
  * where it won't get overwritten by kernel, initrd or atags.
  */
 .text : { *(.text) }
 .data : { *(.data) }
 .bss : { *(.bss) }

 . = STACKTOP;
 stacktop = .;

	. = S_SVC_STACKTOP;
	s_svc_stacktop = .;
	
	. = MON_STACKSTOP;
	mon_stacktop = .;
	
	. = NS_SVC_STACKTOP;
	ns_svc_stacktop = .;
}
