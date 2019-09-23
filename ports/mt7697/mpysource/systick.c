/*
 * This file is part of the MicroPython porting to linkit 7697 project
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 "Song Yang" <onionys@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "top.h"
#include "hal.h"

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/obj.h"

#include "systick.h"


#define THIS_OVERHEAD 10
#define PEND_OVERHEAD 300

#define _RAISE_OS_ERR(x) nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, x ));

void mp_hal_delay_ms(mp_uint_t ms){
	uint64_t us = (uint64_t)(ms * 1000);
	uint64_t dt;
	uint64_t t0 = (uint64_t) mp_hal_ticks_us();
	for(;;){
		uint64_t t1 = (uint64_t) mp_hal_ticks_us();
		dt = t1 - t0;
		if(dt + portTICK_PERIOD_MS * 1000 >= us){
			break;
		}
		MICROPY_EVENT_POLL_HOOK
		ulTaskNotifyTake(pdFALSE, 1);
	}
	if(dt < us){
		mp_hal_delay_us(us - dt);
	}
}



void mp_hal_delay_us(mp_uint_t us){
	if(us < THIS_OVERHEAD){
		return;
	}

	us -= THIS_OVERHEAD;
	mp_uint_t t0 = mp_hal_ticks_us();
	for(;;){
		uint32_t dt = mp_hal_ticks_us() - t0;
		if(dt >= us){
			return ;
		}
		if((dt + PEND_OVERHEAD) < us){
			mp_handle_pending();
		}
	}
}



mp_uint_t mp_hal_ticks_ms(void){
	return mp_hal_ticks_us() / 1000;
}




mp_uint_t mp_hal_ticks_us(void){
	uint32_t v;
	if(HAL_GPT_STATUS_OK != hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &v))
		_RAISE_OS_ERR("mp_hal_tick_ticks_us() Error!");
	return v;
}


mp_uint_t mp_hal_ticks_cpu(void){
	uint32_t v;
	if(HAL_GPT_STATUS_OK != hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_BUS, &v))
		_RAISE_OS_ERR("mp_hal_tick_ticks_cpu() Error!");
	return v;
}


void hw_system_clock_config(void)
{
    top_xtal_init();
	cmnCpuClkConfigureTo192M();
	cmnSerialFlashClkConfTo64M();
}

void software_reset(void){
	hal_wdt_software_reset();
}
