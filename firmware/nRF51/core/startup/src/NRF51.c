/***************************************************************
 *
 * OpenBeacon.org - nRF51 Exception Handlers
 *
 * Copyright 2013 Milosch Meriac <meriac@openbeacon.de>
 *
 ***************************************************************

 This file is part of the OpenBeacon.org active RFID firmware

 OpenBeacon is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenBeacon is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

*/
#include <openbeacon.h>
#include "nrf5.h"

void ResetDefaultHandler(void) __attribute__ ((noreturn));

//*****************************************************************************
//
// Forward declaration of the specific IRQ handlers. These are aliased
// to the IntDefaultHandler, which is a 'forever' loop. When the application
// defines a handler (with the same name), this will automatically take.
// precedence over these weak definitions
//
//*****************************************************************************
void Reset_Handler(void) ALIAS(ResetDefaultHandler);
void NMI_Handler(void) ALIAS(IntDefaultHandler);
void HardFault_Handler(void) ALIAS(IntDefaultHandler);
void SVC_Handler(void) ALIAS(IntDefaultHandler);
void PendSV_Handler(void) ALIAS(IntDefaultHandler);
void SysTick_Handler(void) ALIAS(IntDefaultHandler);
void POWER_CLOCK_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void RADIO_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void UART0_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void SPI0_TWI0_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void SPI1_TWI1_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void GPIOTE_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void ADC_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void TIMER0_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void TIMER1_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void TIMER2_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void RTC0_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void TEMP_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void RNG_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void ECB_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void CCM_AAR_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void WDT_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void RTC1_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void QDEC_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void LPCOMP_COMP_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void SWI0_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void SWI1_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void SWI2_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void SWI3_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void SWI4_IRQ_Handler(void) ALIAS(IntDefaultHandler);
void SWI5_IRQ_Handler(void) ALIAS(IntDefaultHandler);


//*****************************************************************************
//
// External declaration for the pointer to the stack top from the Linker Script
//
//*****************************************************************************
extern void __stack_end__(void);

/* External Interrupts */
__attribute__ ((section(".isr_vector")))
void (*const g_pfnVectors[]) (void) =
{
	__stack_end__,
	Reset_Handler,               /* -15 */
	NMI_Handler,                 /* -14 */
	HardFault_Handler,           /* -13 */
	0,                           /* -12 */
	0,                           /* -11 */
	0,                           /* -10 */
	0,                           /* - 9 */
	0,                           /* - 8 */
	0,                           /* - 7 */
	0,                           /* - 6 */
	SVC_Handler,                 /* - 5 */
	0,                           /* - 4 */
	0,                           /* - 3 */
	PendSV_Handler,              /* - 2 */
	SysTick_Handler,             /* - 1 */
	POWER_CLOCK_IRQ_Handler,     /*   0 */
	RADIO_IRQ_Handler,           /*   1 */
	UART0_IRQ_Handler,           /*   2 */
	SPI0_TWI0_IRQ_Handler,       /*   3 */
	SPI1_TWI1_IRQ_Handler,       /*   4 */
	0,                           /*   5 */
	GPIOTE_IRQ_Handler,          /*   6 */
	ADC_IRQ_Handler,             /*   7 */
	TIMER0_IRQ_Handler,          /*   8 */
	TIMER1_IRQ_Handler,          /*   9 */
	TIMER2_IRQ_Handler,          /*  10 */
	RTC0_IRQ_Handler,            /*  11 */
	TEMP_IRQ_Handler,            /*  12 */
	RNG_IRQ_Handler,             /*  13 */
	ECB_IRQ_Handler,             /*  14 */
	CCM_AAR_IRQ_Handler,         /*  15 */
	WDT_IRQ_Handler,             /*  16 */
	RTC1_IRQ_Handler,            /*  17 */
	QDEC_IRQ_Handler,            /*  18 */
	LPCOMP_COMP_IRQ_Handler,     /*  19 */
	SWI0_IRQ_Handler,            /*  20 */
	SWI1_IRQ_Handler,            /*  21 */
	SWI2_IRQ_Handler,            /*  22 */
	SWI3_IRQ_Handler,            /*  23 */
	SWI4_IRQ_Handler,            /*  24 */
	SWI5_IRQ_Handler,            /*  25 */
	0,                           /*  26 */
	0,                           /*  27 */
	0,                           /*  28 */
	0,                           /*  29 */
	0,                           /*  30 */
	0                            /*  31 */
};

//*****************************************************************************
//
// The following are constructs created by the linker, indicating where the
// the "data" and "bss" segments reside in memory.  The initializers for the
// for the "data" segment resides immediately following the "text" segment.
//
//*****************************************************************************
extern unsigned long __end_of_text__;
extern unsigned long __data_beg__;
extern unsigned long __data_end__;
extern unsigned long __bss_beg__;
extern unsigned long __bss_end__;

//*****************************************************************************
//
// This is the code that gets called when the processor first starts execution
// following a reset event.  Only the absolutely necessary set is performed,
// after which the application supplied entry() routine is called.  Any fancy
// actions (such as making decisions based on the reset cause register, and
// resetting the bits in that register) are left solely in the hands of the
// application.
//
//*****************************************************************************
void ResetDefaultHandler(void)
{
	unsigned long *src, *dst;

	// enable RAM first
	NRF_POWER->RAMON |= 0xF;

	//
	// Copy the data segment initializers from flash to SRAM.
	//
	src = &__end_of_text__;
	for(dst = &__data_beg__; dst < &__data_end__;)
		*dst++ = *src++;

	//
	// Zero fill the bss segment.  This is done with inline assembly
	// since this will clear the value of pulDest if it is not kept in
	// a register.
	//
	for(dst = &__bss_beg__; dst < &__bss_end__;)
		*dst++ = 0;

#ifdef __USE_CMSIS
	SystemInit();
#endif

	main_entry();

	//
	// main() shouldn't return, but if it does, we'll just enter an infinite loop.
	//
	while (1){
	}
}

//*****************************************************************************
//
// Processor ends up here if an unexpected interrupt occurs or a handler
// is not present in the application code.
//
//*****************************************************************************
void IntDefaultHandler(void)
{
	//
	// Go into an infinite loop.
	//
	while (1) {
	}
}
