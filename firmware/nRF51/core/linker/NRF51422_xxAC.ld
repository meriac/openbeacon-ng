MEMORY
{
	flash : ORIGIN = 0x00000000, LENGTH = 256K
	ram   : ORIGIN = 0x20000000, LENGTH = 32K
}

INCLUDE "../core/linker/nrf5.ld"