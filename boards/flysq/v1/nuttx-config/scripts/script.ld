/*
*****************************************************************************
**

**  File        : script.ld
**
**  Abstract    : Linker script for STM32F407ZG Device with
**                1024KByte FLASH, 128KByte RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Environment : Atollic TrueSTUDIO(R)
**
**  Distribution: The file is distributed as is, without any warranty
**                of any kind.
**
**  (c)Copyright Atollic AB.
**  You may use this file as-is or modify it according to the needs of your
**  project. This file may only be built (assembled or compiled and linked)
**  using the Atollic TrueSTUDIO(R) product. The use of this file together
**  with other tools than Atollic TrueSTUDIO(R) is not permitted.
**
*****************************************************************************
*/


/* Highest address of the user mode stack */
_estack = 0x20020000;    /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x2000;      /* required amount of heap  */
_Min_Stack_Size = 0x2000; /* required amount of stack */

/* Specify the memory areas */
MEMORY
{
sram (xrw)      : ORIGIN = 0x20000000, LENGTH = 128K
ccsram (rw)      : ORIGIN = 0x10000000, LENGTH = 64K
flash (rx)      : ORIGIN = 0x8000000, LENGTH = 1024K
}

OUTPUT_ARCH(arm)

ENTRY(__start)		/* treat __start as the anchor for dead code stripping */
EXTERN(_vectors)	/* force the vectors to be included in the output */

/*
 * Ensure that abort() is present in the final object.  The exception handling
 * code pulled in by libgcc.a requires it (and that code cannot be easily avoided).
 */
EXTERN(abort)
EXTERN(_bootdelay_signature)

SECTIONS
{
	.text : {
		_stext = ABSOLUTE(.);
		*(.vectors)
		. = ALIGN(32);
		/*
		This signature provides the bootloader with a way to delay booting
		*/
		_bootdelay_signature = ABSOLUTE(.);
		FILL(0xffecc2925d7d05c5)
		. += 8;
		*(.text .text.*)
		*(.fixup)
		*(.gnu.warning)
		*(.rodata .rodata.*)
		*(.gnu.linkonce.t.*)
		*(.got)
		*(.gcc_except_table)
		*(.gnu.linkonce.r.*)
		_etext = ABSOLUTE(.);

	} > flash

	/*
	 * Init functions (static constructors and the like)
	 */
        .init_section : {
                _sinit = ABSOLUTE(.);
                KEEP(*(.init_array .init_array.*))
                _einit = ABSOLUTE(.);
        } > flash


	.ARM.extab : {
		*(.ARM.extab*)
	} > flash

	__exidx_start = ABSOLUTE(.);
	.ARM.exidx : {
		*(.ARM.exidx*)
	} > flash
	__exidx_end = ABSOLUTE(.);

	_eronly = ABSOLUTE(.);

	.data : {
		_sdata = ABSOLUTE(.);
		*(.data .data.*)
		*(.gnu.linkonce.d.*)
		CONSTRUCTORS
		_edata = ABSOLUTE(.);
	} > sram AT > flash

	.bss : {
		_sbss = ABSOLUTE(.);
		*(.bss .bss.*)
		*(.gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = ABSOLUTE(.);
	} > sram

	/* Stabs debugging sections. */
	.stab 0 : { *(.stab) }
	.stabstr 0 : { *(.stabstr) }
	.stab.excl 0 : { *(.stab.excl) }
	.stab.exclstr 0 : { *(.stab.exclstr) }
	.stab.index 0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }
	.comment 0 : { *(.comment) }
	.debug_abbrev 0 : { *(.debug_abbrev) }
	.debug_info 0 : { *(.debug_info) }
	.debug_line 0 : { *(.debug_line) }
	.debug_pubnames 0 : { *(.debug_pubnames) }
	.debug_aranges 0 : { *(.debug_aranges) }
}