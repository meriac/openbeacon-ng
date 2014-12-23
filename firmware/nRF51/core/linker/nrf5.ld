ENTRY(Reset_Handler)

SECTIONS
{
	. = 0;
	startup : { *(.startup)} >flash

	prog : 
	{
		KEEP(*(.softdevice.text))
		KEEP(*(.isr_vector))
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata*)
		*(.glue_7)
		*(.glue_7t)
	} >flash

	__end_of_text__ = .;

	.persistent (NOLOAD):
	{
		KEEP(*(.softdevice.bss))
		__persistent_beg__ = .;
		*(.persistent)
		*(.persistent.*)
		__persistent_end__ = .;
		*(.persistent_manage)
		*(.persistent_manage.*)
	} >ram

	.data : 
	{
		__data_beg__ = .;
		__data_beg_src__ = __end_of_text__;
		*(.data)
		*(.data.*)
		*(.fastrun)
		*(.ramfunc)
		__data_end__ = .;
	} >ram AT>flash
  /* .ARM.exidx is sorted, so has to go in its own output section.  */
  __exidx_start = .;
  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } >ram AT>flash
  __exidx_end = .;

	.bss (NOLOAD):
	{
		__bss_beg__ = .;
		*(.bss)
		*(.bss.*)
		__bss_end__ = .;
	} >ram

	/* Align here to ensure that the .bss section occupies space up to
	_end.  Align after .bss to ensure correct alignment even if the
	.bss section disappears because there are no input sections.  */
	. = ALIGN(32 / 8);
	_end = .;
	_bss_end__ = . ; __bss_end__ = . ; __end__ = . ;
	PROVIDE (end = .);
	PROVIDE (__stack_end__ = ORIGIN(ram) + LENGTH(ram));
}
	. = ALIGN(32 / 8);
