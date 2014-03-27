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
#ifndef __NRF5_H__
#define __NRF5_H__

extern void Reset_Handler(void);
extern void NMI_Handler(void);
extern void HardFault_Handler(void);
extern void SVC_Handler(void);
extern void PendSV_Handler(void);
extern void SysTick_Handler(void);
extern void POWER_CLOCK_IRQ_Handler(void);
extern void RADIO_IRQ_Handler(void);
extern void UART0_IRQ_Handler(void);
extern void SPI0_TWI0_IRQ_Handler(void);
extern void SPI1_TWI1_IRQ_Handler(void);
extern void GPIOTE_IRQ_Handler(void);
extern void ADC_IRQ_Handler(void);
extern void TIMER0_IRQ_Handler(void);
extern void TIMER1_IRQ_Handler(void);
extern void TIMER2_IRQ_Handler(void);
extern void RTC0_IRQ_Handler(void);
extern void TEMP_IRQ_Handler(void);
extern void RNG_IRQ_Handler(void);
extern void ECB_IRQ_Handler(void);
extern void CCM_AAR_IRQ_Handler(void);
extern void WDT_IRQ_Handler(void);
extern void RTC1_IRQ_Handler(void);
extern void QDEC_IRQ_Handler(void);
extern void LPCOMP_COMP_IRQ_Handler(void);
extern void SWI0_IRQ_Handler(void);
extern void SWI1_IRQ_Handler(void);
extern void SWI2_IRQ_Handler(void);
extern void SWI3_IRQ_Handler(void);
extern void SWI4_IRQ_Handler(void);
extern void SWI5_IRQ_Handler(void);

#endif/*__NRF5_H__*/
