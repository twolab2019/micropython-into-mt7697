/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Damien P. George
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

#include <stdio.h>
#include "hal.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "modmachine.h"
#include "extmod/machine_mem.h"
#include "extmod/machine_pinbase.h"
#include "extmod/machine_signal.h"
#include "extmod/machine_spi.h"
#include "extmod/machine_pulse.h"
#include "lib/utils/pyexec.h"

void machine_init(void){
	;// TODO
}




void machine_deinit(void){
	;// TODO
}




STATIC mp_obj_t machine_info(void){
	// TODO
	printf("module machine info: Uncompleted function!\r\n");
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_info_obj, machine_info);





STATIC mp_obj_t machine_reset(void){
	// TODO
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_obj, machine_reset);




STATIC mp_obj_t machine_soft_reset(void){
	pyexec_system_exit = PYEXEC_FORCED_EXIT;
	nlr_raise(mp_obj_new_exception(&mp_type_SystemExit));
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_soft_reset_obj, machine_soft_reset);




STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__),      MP_ROM_QSTR(MP_QSTR_umachine) },
	{ MP_ROM_QSTR(MP_QSTR_info),          MP_ROM_PTR(&machine_info_obj) },
	{ MP_ROM_QSTR(MP_QSTR_reset),         MP_ROM_PTR(&machine_reset_obj) },
	{ MP_ROM_QSTR(MP_QSTR_soft_reset),    MP_ROM_PTR(&machine_soft_reset_obj) },
	// { MP_ROM_QSTR(MP_QSTR_enable_irq),    MP_ROM_PTR(&machine_enable_irq_obj) },
	// { MP_ROM_QSTR(MP_QSTR_disable_irq),    MP_ROM_PTR(&machine_disable_irq_obj) },
	// { MP_ROM_QSTR(MP_QSTR_freq),    MP_ROM_PTR(&machine_freq_obj) },
	// { MP_ROM_QSTR(MP_QSTR_idle),    MP_ROM_PTR(&machine_idle_obj) },
	// { MP_ROM_QSTR(MP_QSTR_sleep),    MP_ROM_PTR(&machine_sleep_obj) },
	// { MP_ROM_QSTR(MP_QSTR_deepsleep),    MP_ROM_PTR(&machine_deepsleep_obj) },
	// { MP_ROM_QSTR(MP_QSTR_reset_cause),    MP_ROM_PTR(&machine_reset_cause_obj) },
	// { MP_ROM_QSTR(MP_QSTR_wake_reason),    MP_ROM_PTR(&machine_wake_reason_obj) },

	{ MP_ROM_QSTR(MP_QSTR_Pin),           MP_ROM_PTR(&machine_pin_type) },
	// { MP_ROM_QSTR(MP_QSTR_RTC),           MP_ROM_PTR(&machine_rtc_type) },
	// { MP_ROM_QSTR(MP_QSTR_PWM),           MP_ROM_PTR(&machine_pwm_type) },
	// { MP_ROM_QSTR(MP_QSTR_ADC),           MP_ROM_PTR(&machine_adc_type) },
	{ MP_ROM_QSTR(MP_QSTR_SPI),           MP_ROM_PTR(&mp_machine_soft_spi_type) },
	{ MP_ROM_QSTR(MP_QSTR_I2C),           MP_ROM_PTR(&machine_i2c_type) },
	// { MP_ROM_QSTR(MP_QSTR_Timer),         MP_ROM_PTR(&machine_timer_type) },
	// { MP_ROM_QSTR(MP_QSTR_WDT),           MP_ROM_PTR(&machine_wdt_type) },

    // { MP_ROM_QSTR(MP_QSTR_time_pulse_us), MP_ROM_PTR(&machine_time_pulse_us_obj) },
	// -- TODO 
	// { MP_ROM_QSTR(MP_QSTR_SPI) ,          MP_ROM_PTR(&machine_hard_spi_type) },
	// { MP_ROM_QSTR(MP_QSTR_I2C) ,          MP_ROM_PTR(&machine_i2c_type) },
	// { MP_ROM_QSTR(MP_QSTR_UART) ,         MP_ROM_PTR(&machine_uart_type) },

    //{ MP_ROM_QSTR(MP_QSTR_mem8),          MP_ROM_PTR(&machine_mem8_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_mem16),         MP_ROM_PTR(&machine_mem16_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_mem32),         MP_ROM_PTR(&machine_mem32_obj) },

//    { MP_ROM_QSTR(MP_QSTR_PinBase), MP_ROM_PTR(&machine_pinbase_type) },
//    { MP_ROM_QSTR(MP_QSTR_Signal), MP_ROM_PTR(&machine_signal_type) },
	/* CONSTANTS */
    // { MP_ROM_QSTR(MP_QSTR_IDLE),             MP_ROM_INT(?) },
    // { MP_ROM_QSTR(MP_QSTR_SLEEP),            MP_ROM_INT(?) },
    // { MP_ROM_QSTR(MP_QSTR_DEEPSLEEP),        MP_ROM_INT(?) },
    // { MP_ROM_QSTR(MP_QSTR_PWR_ON_RESET),     MP_ROM_INT(?) },
    // { MP_ROM_QSTR(MP_QSTR_HARD_RESET),       MP_ROM_INT(?) },
    // { MP_ROM_QSTR(MP_QSTR_SOFT_RESET),       MP_ROM_INT(?) },
    // { MP_ROM_QSTR(MP_QSTR_WDT_RESET),        MP_ROM_INT(?) },
    // { MP_ROM_QSTR(MP_QSTR_DEEPSLEEP_RESET),  MP_ROM_INT(?) },
    // { MP_ROM_QSTR(MP_QSTR_NETWORK_WAKE),     MP_ROM_INT(?) },
    // { MP_ROM_QSTR(MP_QSTR_PIN_WAKE),         MP_ROM_INT(?) },
    // { MP_ROM_QSTR(MP_QSTR_RTC_WAKE),         MP_ROM_INT(?) },

};

STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

const mp_obj_module_t mp_module_machine = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&machine_module_globals,
};
