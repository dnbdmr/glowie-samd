/*
 * Copyright (c) 2020, DNBDMR
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

#include <stdbool.h>
#include "sam.h"
#include "hal_gpio.h"
#include "dma.h"

/*- Data --------------------------------------------------------------------*/
extern uint8_t out_buf[];

volatile DmacDescriptor descarray[1];
DmacDescriptor descarray_wb[1];

/*- Functions --------------------------------------------------------------*/
void dma_init(void)
{
	descarray[0].BTCTRL.reg = DMAC_BTCTRL_VALID | DMAC_BTCTRL_SRCINC | DMAC_BTCTRL_STEPSEL;
	descarray[0].BTCNT.reg = 600;
	descarray[0].DSTADDR.reg = (uint32_t)&(SERCOM0->SPI.DATA.reg);
	descarray[0].SRCADDR.reg = (uint32_t)out_buf + 600;
	descarray[0].DESCADDR.reg = 0;

	DMAC->BASEADDR.reg = (uint32_t)descarray;
	DMAC->WRBADDR.reg = (uint32_t)descarray_wb;

	PM->AHBMASK.bit.DMAC_ = 1;
	PM->APBBMASK.bit.DMAC_ = 1;
	DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xf);

	DMAC->CHID.reg = 0; // select channel 0
	DMAC->CHCTRLB.reg = DMAC_CHCTRLB_LVL(0) | DMAC_CHCTRLB_TRIGSRC(SERCOM0_DMAC_ID_TX) | DMAC_CHCTRLB_TRIGACT_BEAT;
}

void dma_ch_enable(uint8_t channel)
{
	DMAC->CHID.reg = channel; // select channel
	DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;
}

void dma_ch_disable(uint8_t channel)
{
	DMAC->CHID.reg = channel; // select channel
	DMAC->CHCTRLA.reg &= !(DMAC_CHCTRLA_ENABLE);
}

bool dma_ch_enabled(uint8_t channel)
{
	DMAC->CHID.reg = channel; // select channel
	if (DMAC->CHCTRLA.bit.ENABLE)
		return true;
	else
		return false;
}
