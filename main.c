/*
 * Copyright (c) 2016-2017, Alex Taradov <alex@taradov.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *	  this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *	  derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*- Includes ----------------------------------------------------------------*/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>
#include <string.h>
#include "sam.h"
#include "hal_gpio.h"
#include "tusb.h"
#include "light_ws2812_cortex.h"
#include "utils.h"

/*- Definitions -------------------------------------------------------------*/
HAL_GPIO_PIN(LED2,	A, 27)	// On board LED
HAL_GPIO_PIN(NEOPIN,	A, 2)	// Neopixel output

/*- Implementations ---------------------------------------------------------*/

volatile uint32_t msticks = 0;

void SysTick_Handler(void)
{
	msticks++;
}

uint32_t millis(void)
{
	uint32_t m;
	__disable_irq();
	__DMB();
	m = msticks;
	__enable_irq();
	__DMB();
	return m;
}

void delay_us(uint32_t us)
{
	if (!us || (us >= SysTick->LOAD))
		return;
	if(!(SysTick->CTRL & SysTick_CTRL_ENABLE_Msk))
		return;
	us = F_CPU/1000000*us;
	uint32_t time = SysTick->VAL;
	while ((time - SysTick->VAL) < us);
}

//-----------------------------------------------------------------------------
// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
	(void) itf;

	// connected
	if ( dtr && rts )
	{
		// print initial message when connected
		//tud_cdc_write_str("Hello!\n");
	}

	//Reset into bootloader when baud is 1200 and dtr unasserted
	if (!dtr) {
		cdc_line_coding_t lc;
		tud_cdc_get_line_coding(&lc);
		if (lc.bit_rate == 1200) {
			NVIC_SystemReset();
		}
	}
}

//-----------------------------------------------------------------------------

/* Retrieves full line from cdc, returns true when found */
uint8_t cdc_task(uint8_t line[], uint8_t max)
{
	static uint8_t pos = 0;
	uint8_t success = 0;

	if (tud_cdc_connected() && tud_cdc_available()) {	// connected and there are data available
		uint8_t buf[64];
		uint8_t count = tud_cdc_read(buf, sizeof(buf));

		for (uint8_t i=0; i<count; i++) {
			tud_cdc_write_char(buf[i]);
			if (pos < max-1) {
				if ((line[pos] = buf[i]) == '\n') {
					success = 1;
				}
				pos++;
			}
		}
	}

	tud_cdc_write_flush(); // Freeze without this

	if (success) {
		success = 0;
		line[pos] = '\0';
		pos = 0;
		return 1;
	}
	else
		return 0;
}

const char help_msg[] = \
						"Tiny usb test commands:\n" \
						"b [ms]\ttimer blink rate\n" \
						"d [0-255]\tled pwm\n" \
						"h\tprint humidity\n" \
						"t\tprint local temp\n" \
						"o\tprint remote temp\n" \
						"c\tprint to debug uart\n" \
						"d\tDMA uart enable\n" \
						"D\tDMA uart disable\n";

void print_help(void)
{
	size_t len = strlen(help_msg);
	size_t pos = 0;
	while (pos < len) {
		uint32_t avail = tud_cdc_write_available();
		if ((len - pos) > avail) {
		   tud_cdc_write(&help_msg[pos], avail);
		} else {
			tud_cdc_write(&help_msg[pos], len - pos);
		}	
		pos += avail;
		tud_task();
	}
}

int atoi2(const char *str)
{
	if (*str == '\0')
		return 0;

	int res = 0;  // Initialize result
	int sign = 1;  // Initialize sign as positive
	int i = 0;   // Initialize index of first digit

	while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\v')
		i++; // Skip whitespace

	if (str[i] == '-') {
		sign = -1;
		i++;
	}

	for (; str[i] != '\0'; ++i)	{
		if (str[i] < '0' || str[i] > '9') // If string contain character it will terminate
			break; 
		res = res*10 + str[i] - '0';
	}

	return sign*res;
}

//--------------------------------------------------------------------------

void rgb_wheel( uint8_t led[], uint8_t pos)
{
	if (pos < 85) {
		//led[0] = 85 - pos;
		led[0] = 255 - pos * 3;
		led[1] = pos * 3;
		led[2] = 0;
		return;
	}
	if (pos < 170) {
		pos -= 85;
		led[0] = 0;
		led[1] = 255 - pos * 3;
		led[2] = pos;
		return;
	}
	pos -= 170;
	led[0] = pos * 3;
	led[1] = 0;
	led[2] = 85 - pos;
	return;
}

uint8_t gamma8[] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
        2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
        5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
        10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
        17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
        25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
        37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
        51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
        69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
        90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
        115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
        144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
        177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
        215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

#define NUMPIX	50
#define NUMBYTES	NUMPIX*3
#define MAXDELAY 0x1F	// 32s total up+down
#define MAXHOLD	0xFF	// 255ms
#define	MAXWAIT	0xFFF	// 4.096ms

enum rgb_states {
	state_up,
	state_hold,
	state_down,
	state_wait
};

struct rand_RGB {
	uint8_t r_max;
	uint8_t g_max;
	uint8_t b_max;
	uint8_t r_current;
	uint8_t g_current;
	uint8_t b_current;

	uint16_t delay_max;
	uint16_t delay_current;
	uint16_t delay_hold;
	uint16_t delay_wait;

	enum rgb_states state;
} pixels[NUMPIX] = { 0 };

uint8_t out_buf[NUMBYTES] = { 0 };

void neo_init(struct rand_RGB *pixel)
{
	pixel->r_max = rand() & 0xFF;
	pixel->g_max = rand() & 0xFF;
	pixel->b_max = LIMIT(rand() & 0xFF, 100);
	pixel->delay_max = rand() & MAXDELAY;
	pixel->delay_hold = rand() & MAXHOLD;
	pixel->delay_wait = rand() & MAXWAIT;
}

void neo_init_all(void)
{
	for (uint8_t i = 0; i < NUMPIX; i++) {
		neo_init(&pixels[i]);
	}

	for (uint8_t i = 0; i < NUMPIX; i++) {
		out_buf[(i*3)] = gamma8[pixels[i].g_current];
		out_buf[(i*3)+1] = gamma8[pixels[i].r_current];
		out_buf[(i*3)+2] = gamma8[pixels[i].b_current];
	}
	ws2812_sendarray(out_buf, NUMBYTES);
	delay_us(200);
}

void neo_task(void)
{
	for (uint8_t i = 0; i < NUMPIX; i++) {
		if (pixels[i].state == state_up) {
			if (pixels[i].delay_current < pixels[i].delay_max) {
				pixels[i].delay_current++;
				continue;
			}
			pixels[i].delay_current = 0;

			if (pixels[i].r_current < pixels[i].r_max)
				pixels[i].r_current++;
			if (pixels[i].g_current < pixels[i].g_max)
				pixels[i].g_current++;
			if (pixels[i].b_current < pixels[i].b_max)
				pixels[i].b_current++;

			if ((pixels[i].r_current >= pixels[i].r_max) &&
					(pixels[i].g_current >= pixels[i].g_max) &&
					(pixels[i].b_current >= pixels[i].b_max))
				pixels[i].state = state_hold;
		}
		else if (pixels[i].state == state_hold) {
			if (pixels[i].delay_hold) {
				pixels[i].delay_hold--;
				continue;
			}
			pixels[i].state = state_down;
		}
		else if (pixels[i].state == state_down) {
			if (pixels[i].delay_current < pixels[i].delay_max) {
				pixels[i].delay_current++;
				continue;
			}
			pixels[i].delay_current = 0;

			if (pixels[i].r_current)
				pixels[i].r_current--;
			if (pixels[i].g_current)
				pixels[i].g_current--;
			if (pixels[i].b_current)
				pixels[i].b_current--;

			if (!(pixels[i].r_current) && !(pixels[i].g_current) && !(pixels[i].b_current))
				pixels[i].state = state_wait;
		}
		else if (pixels[i].state == state_wait) {
			if (pixels[i].delay_wait) {
				pixels[i].delay_wait--;
				continue;
			}

			neo_init(&pixels[i]);
			pixels[i].state = state_up;
		}
		else
			pixels[i].state = state_up;
	}

	for (uint8_t i = 0; i < NUMPIX; i++) {
		out_buf[(i*3)] = gamma8[pixels[i].g_current];
		out_buf[(i*3)+1] = gamma8[pixels[i].r_current];
		out_buf[(i*3)+2] = gamma8[pixels[i].b_current];
	}

	ws2812_sendarray(out_buf, NUMBYTES);
}

//-----------------------------------------------------------------------------
int main(void)
{
	srand(2);
	neo_init_all();
	tusb_init();

	HAL_GPIO_LED2_out();
	HAL_GPIO_LED2_clr();

	uint8_t line[25];

	HAL_GPIO_NEOPIN_out();
	uint32_t neo_time = millis();
	//uint8_t neo_pos = 0;
	uint32_t blink_time = millis();
	uint32_t blink_rate = 500;

	while (1)
	{
		tud_task();

		if (cdc_task(line, 25)) {
			if (line[0] == 'b') {
				uint32_t ms = atoi2((char *)&line[1]);
				if (ms > 0 && ms < 50000)
					blink_rate = ms;
			}
			else if (line[0] == 'r') {
				char s[20];
				itoa(rand()&0xFF, s, 10);
				tud_cdc_write_str(s);
				tud_cdc_write_char('\n');
			}
			else if (line[0] == '?') {
				print_help();
			}
		}

		// Should be greater than 30us * NUMPIX
		if (millis() - neo_time >= 2) {
			neo_task();
			neo_time = millis();
		}
		/*
		if (millis() - neo_time >= 0x1F) {
			neo_time = millis();
			for (uint8_t i = 0; i < NUMPIX; i++) {
				rgb_wheel(&out_buf[i*3], neo_pos+5*i);
			}
			neo_pos+=1;
			ws2812_sendarray(out_buf, NUMBYTES);
		}
		*/

		if (millis() - blink_time >= blink_rate) {
			HAL_GPIO_LED2_toggle();
			blink_time = millis();
		}

	}

	return 0;
}
