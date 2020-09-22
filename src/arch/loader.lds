OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)
SECTIONS
{
  . = 0x00000000;
  _text = .;
  .text : { 

      *(.text)
	      . = ALIGN(8);
	  __init_start = .;
      *(.init_stardust)
	  __init_end = .;
	  *(.gnu.warning)
  } = 0x9090

  _etext = .;

  .rodata : { *(.rodata) *(.rodata.*) }

  .data : {
	*(.data)
	CONSTRUCTORS
	}

  _edata = .;

  . = ALIGN(8192);
  .data.init_task : { *(.data.init_task) }

  . = ALIGN(4096);
  .data.page_aligned : { *(.data.idt) }

  . = ALIGN(32);
  .data.cacheline_aligned : { *(.data.cacheline_aligned) }

  __bss_start = .;
  .bss : {
	*(.bss)
	}
  _end = . ;

  /DISCARD/ : {
	*(.text.exit)
	*(.data.exit)
	*(.exitcall.exit)
	}

  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  .stab.excl 0 : { *(.stab.excl) }
  .stab.exclstr 0 : { *(.stab.exclstr) }
  .stab.index 0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 : { *(.comment) }
}
