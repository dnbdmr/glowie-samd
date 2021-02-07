/*
MODIFIED:
- handler names replaced with more industry-standard ones
- linker memory map locations replaced with Rowley-convention ones
*/

/*
 * Copyright (c) 2016, Alex Taradov <alex@taradov.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include "sam.h"
#include "nvm_data.h"
#include "tusb.h"

//-----------------------------------------------------------------------------
#define DUMMY __attribute__ ((weak, alias ("irq_handler_dummy")))

//-----------------------------------------------------------------------------
void Reset_Handler(void);
#ifndef STARTUP_FROM_RESET
void Reset_Wait(void) {while (1);}
#endif
DUMMY void NMI_Handler(void);
DUMMY void HardFault_Handler(void);
DUMMY void SVC_Handler(void);
DUMMY void PendSV_Handler(void);
DUMMY void SysTick_Handler(void);

DUMMY void PM_Handler(void);
DUMMY void SYSCTRL_Handler(void);
DUMMY void WDT_Handler(void);
DUMMY void RTC_Handler(void);
DUMMY void EIC_Handler(void);
DUMMY void NVMCTRL_Handler(void);
DUMMY void DMAC_Handler(void);
DUMMY void EVSYS_Handler(void);
DUMMY void SERCOM0_Handler(void);
DUMMY void SERCOM1_Handler(void);
DUMMY void SERCOM2_Handler(void);
DUMMY void TCC0_Handler(void);
DUMMY void TC1_Handler(void);
DUMMY void TC2_Handler(void);
DUMMY void ADC_Handler(void);
DUMMY void AC_Handler(void);
DUMMY void DAC_Handler(void);
DUMMY void PTC_Handler(void);

extern int main(void);

extern void __stack_end__(void);
extern unsigned int __data_load_start__;
extern unsigned int __data_start__;
extern unsigned int __data_end__;
extern unsigned int __bss_start__;
extern unsigned int __bss_end__;

//-----------------------------------------------------------------------------
__attribute__ ((used, section(".vectors")))
void (* const vectors[])(void) =
{
  &__stack_end__,            // 0 - Initial Stack Pointer Value

  // Cortex-M0+ handlers
#ifdef STARTUP_FROM_RESET
  Reset_Handler,             // 1 - Reset
#else
  Reset_Wait,
#endif
  NMI_Handler,               // 2 - NMI
  HardFault_Handler,         // 3 - Hard Fault
  0,                         // 4 - Reserved
  0,                         // 5 - Reserved
  0,                         // 6 - Reserved
  0,                         // 7 - Reserved
  0,                         // 8 - Reserved
  0,                         // 9 - Reserved
  0,                         // 10 - Reserved
  SVC_Handler,               // 11 - SVCall
  0,                         // 12 - Reserved
  0,                         // 13 - Reserved
  PendSV_Handler,            // 14 - PendSV
  SysTick_Handler,           // 15 - SysTick

  // Peripheral handlers
  PM_Handler,                // 0 - Power Manager
  SYSCTRL_Handler,           // 1 - System Controller
  WDT_Handler,               // 2 - Watchdog Timer
  RTC_Handler,               // 3 - Real Time Counter
  EIC_Handler,               // 4 - External Interrupt Controller
  NVMCTRL_Handler,           // 5 - Non-Volatile Memory Controller
  DMAC_Handler,              // 6 - Direct Memory Access Controller
  USB_Handler,               // 7 - USB Controller
  EVSYS_Handler,             // 8 - Event System
  SERCOM0_Handler,           // 9 - Serial Communication Interface 0
  SERCOM1_Handler,           // 10 - Serial Communication Interface 1
  SERCOM2_Handler,           // 11 - Serial Communication Interface 2
  TCC0_Handler,              // 12 - Timer/Counter for Control 0
  TC1_Handler,               // 13 - Timer/Counter 1
  TC2_Handler,               // 14 - Timer/Counter 2
  ADC_Handler,               // 15 - Analog-to-Digital Converter
  AC_Handler,                // 16 - Analog Comparator
  DAC_Handler,               // 17 - Digital-to-Analog Converter
  PTC_Handler,               // 18 - Peripheral Touch Controller
};

//-----------------------------------------------------------------------------
void Reset_Handler(void)
{
  unsigned int *src, *dst;

  src = &__data_load_start__;
  dst = &__data_start__;
  while (dst < &__data_end__)
    *dst++ = *src++;

  dst = &__bss_start__;
  while (dst < &__bss_end__)
    *dst++ = 0;

  // Set Flash Wait States to 1 for 3.3V operation @ 48MHz
  NVMCTRL->CTRLB.bit.RWS = 1;

  // See Errata 9905
  SYSCTRL->DFLLCTRL.reg = 0;
  while (0 == (SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY));

  // Set to multiply USB SOF frequency (when USB attached)
  SYSCTRL->DFLLMUL.reg = SYSCTRL_DFLLMUL_MUL(48000);

  // Load factory cals
  SYSCTRL->DFLLVAL.reg = SYSCTRL_DFLLVAL_COARSE(NVM_READ_CAL(DFLL48M_COARSE_CAL)) | 
	  					 SYSCTRL_DFLLVAL_FINE(NVM_READ_CAL(DFLL48M_FINE_CAL));

  SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE | 
	  SYSCTRL_DFLLCTRL_USBCRM | // Set DFLL for USB Clock Recovery Mode
	  SYSCTRL_DFLLCTRL_BPLCKC | // Bypass Coarse Lock, ignored with USBCRM
	  SYSCTRL_DFLLCTRL_CCDIS |  // Disable Chill Cycle
	  SYSCTRL_DFLLCTRL_RUNSTDBY |  // Run during standby for USB wakeup interrupts
	  SYSCTRL_DFLLCTRL_MODE;   // Set Closed Loop Mode

  while (!(SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY)); // Wait for DFLL sync complete

  //Setup Generic Clock Generator 0 with DFLL48M as source:
  GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(0) | GCLK_GENCTRL_SRC(GCLK_SOURCE_DFLL48M) |
	  GCLK_GENCTRL_RUNSTDBY | GCLK_GENCTRL_GENEN;
  while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);

  SysTick_Config(48000); //systick at 1ms

  // Enable USB Clocks
  PM->APBBMASK.reg |= PM_APBBMASK_USB;
  PM->AHBMASK.reg |= PM_AHBMASK_USB;
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID(USB_GCLK_ID) |
	  GCLK_CLKCTRL_GEN(0);

  // Enable USB pins
  PORT->Group[0].PINCFG[PIN_PA24G_USB_DM].bit.PMUXEN = 1;
  PORT->Group[0].PMUX[PIN_PA24G_USB_DM/2].bit.PMUXE = MUX_PA24G_USB_DM;
  PORT->Group[0].PINCFG[PIN_PA25G_USB_DP].bit.PMUXEN = 1;
  PORT->Group[0].PMUX[PIN_PA25G_USB_DP/2].bit.PMUXO = MUX_PA25G_USB_DP;

  main();
  while (1);
}

void USB_Handler(void)
{
	dcd_int_handler(0);
}

//-----------------------------------------------------------------------------
void irq_handler_dummy(void)
{
  while (1);
}

//-----------------------------------------------------------------------------
void _exit(int status)
{
  (void)status;
  while (1);
}
