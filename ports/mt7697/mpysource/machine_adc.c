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
#include <stdio.h>
#include <stdlib.h>

#include "py/runtime.h"
#include "py/obj.h"
#include "modmachine.h"
#include "machine_adc.h"
#include "machine_pin.h"

/* linkit mt7697 HDK ADC 
 *
 * Pin support ADC func: P14, P15, P16, P17
 *
 * measurement range of voltage: 0.0 ~ 2.5 V
 *
 * resolution: 12 bits (read function return value: 0 ~ 4095)
 *
 * input impedance: 10k ohm
 *
 * Usage:
 *     from machine import ADC
 * 	   my_adc = ADC(14)
 *     val = my_adc.read()  # 0 ~ 4095
 *     mV = val * 2500 / 4096
 * */


typedef struct _machine_adc_obj_t {
	mp_obj_base_t base;
	hal_gpio_pin_t gpio_num;
	hal_adc_channel_t adc_ch;
}machine_adc_obj_t;

static bool is_adc_initialized = false;

STATIC mp_obj_t machine_adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw,
        const mp_obj_t *args) {


    mp_arg_check_num(n_args, n_kw, 1, 1, true);
    uint8_t pin_num = mp_obj_get_int(args[0]);

	machine_pin_obj_t *pin_obj_ptr = machine_pin_obj_from_pin_num(pin_num);
	if(pin_obj_ptr->adc_ch == HAL_ADC_CHANNEL_MAX){
		mp_raise_ValueError("invalid pin for ADC.");
		return mp_const_none;
	}

    if (!is_adc_initialized) {
		hal_adc_init();
		is_adc_initialized = true;
    }

    machine_adc_obj_t *self = m_new_obj(machine_adc_obj_t);
	self->base.type = &machine_adc_type;
	self->gpio_num = pin_obj_ptr->gpio_num;
	self->adc_ch = pin_obj_ptr->adc_ch;

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_adc_deinit(mp_obj_t self_in) {
    machine_adc_obj_t *self = self_in;
	if(is_adc_initialized){
		hal_adc_deinit();
		is_adc_initialized = false;
	}
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_adc_deinit_obj, machine_adc_deinit);


// STATIC mp_obj_t machine_adc_mV(mp_obj_t self_in) {
//     machine_adc_obj_t *self = self_in;
// 	uint32_t val;
// 	if(!is_adc_initialized){
// 		mp_raise_msg(&mp_type_OSError, "ADC device not initialized!");
// 	}
// 	hal_adc_get_data_polling(self->adc_ch,&val);
// 	val = (val * 2500)/4096;
//     return MP_OBJ_NEW_SMALL_INT(val);
// }
// MP_DEFINE_CONST_FUN_OBJ_1(machine_adc_mV_obj, machine_adc_mV);

STATIC mp_obj_t machine_adc_read(mp_obj_t self_in) {
    machine_adc_obj_t *self = self_in;
	uint32_t val;
	if(!is_adc_initialized){
		mp_raise_msg(&mp_type_OSError, "ADC device not initialized!");
	}
	hal_adc_get_data_polling(self->adc_ch,&val);
    return MP_OBJ_NEW_SMALL_INT(val);
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_adc_read_obj, machine_adc_read);

STATIC void machine_adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_adc_obj_t *self = self_in;
	machine_pin_obj_t *pin_obj_ptr = machine_pin_obj_from_gpio_num(self->gpio_num);
    mp_printf(print, "ADC(Pin(%u))", pin_obj_ptr->pin_num);
}


STATIC const mp_rom_map_elem_t machine_adc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&machine_adc_read_obj) },
    // { MP_ROM_QSTR(MP_QSTR_mV), MP_ROM_PTR(&machine_adc_mV_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_adc_deinit_obj) },
};

STATIC MP_DEFINE_CONST_DICT(machine_adc_locals_dict, machine_adc_locals_dict_table);

const mp_obj_type_t machine_adc_type = {
    { &mp_type_type },
    .name = MP_QSTR_ADC,
    .print = machine_adc_print,
    .make_new = machine_adc_make_new,
    .locals_dict = (mp_obj_t)&machine_adc_locals_dict,
};
