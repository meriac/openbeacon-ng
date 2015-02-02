/***************************************************************
 *
 * OpenBeacon.org - nRF51 Main Entry
 *
 * Copyright 2013-2014 Milosch Meriac <meriac@openbeacon.de>
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
#include <nrf_sdm.h>
#include <ble_6lowpan.h>

static const eui64_t g_eui = {{0x02, 0x01, 0x02, 0xFF, 0xFE, 0x03, 0x04, 0x05}};

int32_t iot_context_manager_get_by_cid(
	const iot_interface_t * p_interface,
	uint8_t context_id,
	iot_context_t  ** pp_context)
{
	debug_printf("iot_context_manager_get_by_cid (id=%u)\n", context_id);
	return NRF_ERROR_NOT_FOUND;
}

uint32_t nrf51_sdk_mem_alloc(uint8_t ** pp_buffer, uint32_t * p_size)
{
	debug_printf("nrf51_sdk_mem_alloc (size=%u)\n", *p_size);
	return NRF_ERROR_NO_MEM;
}

uint32_t nrf51_sdk_mem_free(uint8_t * p_buffer)
{
	debug_printf("nrf51_sdk_mem_free: 0x%08X\n", p_buffer);
	return NRF_ERROR_INVALID_ADDR;
}

static void event_handler(
	iot_interface_t * p_interface,
	ble_6lowpan_event_t * p_6lo_event)
{
	debug_printf("event=%i\n", p_6lo_event->event_id);
}

static void assert_handler(
	uint32_t pc,
	uint16_t line_number,
	const uint8_t * p_file_name)
{
	debug_printf("%s:%u (0x%08X)\n", p_file_name, line_number, pc);
}

void main_entry(void)
{
	uint32_t res;
	uint8_t enabled;
	volatile int t;
	ble_6lowpan_init_t ble;

	/* enabled LED output */
	nrf_gpio_cfg_output(CONFIG_LED_PIN);
	nrf_gpio_pin_set(CONFIG_LED_PIN);

	/* enabled input pin */
	nrf_gpio_cfg_input(CONFIG_SWITCH_PIN, NRF_GPIO_PIN_NOPULL);

	/* initialize UART */
	uart_init();

	if((res = sd_softdevice_enable(
		NRF_CLOCK_LFCLKSRC_XTAL_20_PPM,
		assert_handler))!=NRF_SUCCESS)
	{
		debug_printf("sd_softdevice_enable failed (0x%02X)\n", res);
		while(1);
	}

	debug_printf("Hello\n");
	ble.p_eui64 = (eui64_t*)&g_eui;
	ble.event_handler = event_handler;
	if((res = ble_6lowpan_init(&ble))!=NRF_SUCCESS)
	{
		debug_printf("ble_6lowpan_init failed (0x%02X)\n", res);
		while(1);
	}

	while(1)
	{
		res = sd_softdevice_is_enabled(&enabled);

		debug_printf("Hello World (0x%08X, %i)\n", res, enabled);
		for(t=0; t<100000; t++);

		if(nrf_gpio_pin_read(CONFIG_SWITCH_PIN))
			nrf_gpio_pin_set(CONFIG_LED_PIN);
		else
			nrf_gpio_pin_clear(CONFIG_LED_PIN);
	}
}
