.section .softdevice.text, "x"
  __softdevice_text_start:
  .incbin "lib/softdevice.bin"
  .align 12
  __softdevice_text_end:

.section .softdevice.bss,"awM",@nobits
  __softdevice_bss_start:
  .space 0x2000
  __softdevice_bss_end:


