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

#include "hal.h"
#include "modmachine.h"
#include "machine_pin.h"
#include "machine_pwm.h"

#include "py/runtime.h"
#include "py/mphal.h"

/*
 * PWM (Pulse Width Modulation)
 * machine.PWM class
 *
 * PWM(pin_no, *, freq=500, duty=0)
 *
 * parameters: 
 *    freq : frequency (Hz) : 400 ~ 19000
 *    duty : duty cycle : 0 ~ 1024
 * 
 * Usage ex:
 *
 * >>> from machine import PWM
 * >>> pwm7 = PWM(7)  
 * >>> pwm7.duty() # get duty cycle 
 * >>> pwm7.freq() # get frequency 
 * >>> pwm7.duty(50) # set duty cycle
 * >>> pwm7.freq(400) # set frequency
 * */
#define HAL_DUTY_CYCLE_FROM_DUTY_CYCLE_BASE1024( x, p ) (  2 * x * p ) / 1024 

typedef struct _machine_pwm_obj_t{
	mp_obj_base_t base;
	hal_gpio_pin_t gpio_num;
	uint32_t duty;
	uint32_t freq;
	uint32_t total_count;
}machine_pwm_obj_t;

static bool is_pwm_clock_source_set = false;
static int hal_clock_source = -1;

STATIC void machine_pwm_init_helper(
		machine_pwm_obj_t * self,
		size_t n_args,
		const mp_obj_t *pos_args,
		mp_map_t *kw_args){

	enum {ARG_freq, ARG_duty, ARG_clock};
	static const mp_arg_t allowed_args[] = {
		{MP_QSTR_freq, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 500} },
		{MP_QSTR_duty, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0 } },
		{MP_QSTR_clock, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_PWM_CLOCK_20MHZ } },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	int freq = args[ARG_freq].u_int;
	int duty = args[ARG_duty].u_int;
	int clock = args[ARG_clock].u_int;

	const machine_pin_obj_t *pin_obj_p = machine_pin_obj_from_gpio_num(self->gpio_num);
	if(pin_obj_p == 0){
		mp_raise_ValueError("pin num error!");
		return;
	}

	if(HAL_GPIO_STATUS_OK != hal_gpio_init(pin_obj_p->gpio_num)){
		nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "hal init fail!"));
		return;
	}

	if(HAL_PINMUX_STATUS_OK != hal_pinmux_set_function(pin_obj_p->gpio_num, pin_obj_p->pwm_pinmux_num)){
		nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "hal pinmux fail!"));
		return;
	}


	if(!is_pwm_clock_source_set){ // -- check if the clk source has been set
		if(HAL_PWM_STATUS_OK != hal_pwm_init(clock)){
			nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "hal set clk source fail!"));
			return;
		}
		is_pwm_clock_source_set = true;
	}
	hal_clock_source = clock;

	if(freq == -1){
		mp_raise_ValueError("pwm freq value required!");
		return;
	}

	self->freq = freq;
	if( HAL_PWM_STATUS_OK != hal_pwm_set_frequency(pin_obj_p->pwm_ch, self->freq, &(self->total_count)) ){
		self->freq = 0;
		mp_raise_ValueError("pwm freq value out of range!");
		return;
	}

	self->duty = (uint32_t) duty;
	if(self->duty > 1024){
		mp_raise_ValueError("pwm duty value out of range!");
		return;
	}
	uint32_t hal_duty_cycle = HAL_DUTY_CYCLE_FROM_DUTY_CYCLE_BASE1024(self->duty, self->total_count);
	if(HAL_PWM_STATUS_OK != hal_pwm_set_duty_cycle(pin_obj_p->pwm_ch, hal_duty_cycle)){
		self->freq = 0;
		self->duty = 0;
		mp_raise_ValueError("pwm duty value out of range!");
		return;
	}

	if(HAL_PWM_STATUS_OK != hal_pwm_start(pin_obj_p->pwm_ch)){
		self->freq = 0;
		self->duty = 0;
		nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "hal pwm start fail!"));
		return;
	}

	return;
}


/*
 * p7 = PWM(7,freq[,duty])
 * freq : 400 ~ 19000 Hz
 * duty : 0~1023 
 * */
STATIC mp_obj_t machine_pwm_make_new(const mp_obj_type_t *type,
		size_t n_args,
		size_t n_kw,
		const mp_obj_t * args){

	mp_arg_check_num(n_args,n_kw,1,MP_OBJ_FUN_ARGS_MAX, true);

	uint8_t pin_num = 0;

	const machine_pin_obj_t * pin_ptr = NULL;
	if(MP_OBJ_IS_TYPE(args[0],&machine_pin_type)){
		pin_ptr = (const machine_pin_obj_t *)args[0];
	}else if (mp_obj_is_integer(args[0])){
		pin_num = (uint8_t) mp_obj_get_int(args[0]);
		pin_ptr = machine_pin_obj_from_pin_num(pin_num);
	}

	machine_pwm_obj_t *self = m_new_obj(machine_pwm_obj_t);
	self->base.type = &machine_pwm_type;
	self->gpio_num = pin_ptr->gpio_num;
	

	mp_map_t kw_args;
	mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
	machine_pwm_init_helper(self, n_args -1, args+1, &kw_args);
	return MP_OBJ_FROM_PTR(self);
}




STATIC mp_obj_t machine_pwm_init(
		size_t n_args,
		const mp_obj_t * args,
		mp_map_t *kw_args){

	machine_pwm_init_helper(args[0], n_args - 1, args + 1, kw_args);
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_pwm_init_obj, 1, machine_pwm_init);




STATIC void machine_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kink){

	machine_pwm_obj_t *self = self_in;
	const machine_pin_obj_t *pin_obj_p = machine_pin_obj_from_gpio_num(self->gpio_num);
	int pin_num = pin_obj_p->pin_num ;
	uint32_t freq = self->freq;
	uint16_t duty_b1024 = self->duty;

	mp_printf(print, "PWM(%u", (uint32_t) pin_num);
	if(self->total_count != 0){
		mp_printf(print, ", freq=%u, duty=%u", freq, duty_b1024);
	}
	mp_printf(print, ")");
}




STATIC mp_obj_t machine_pwm_deinit(mp_obj_t self_in){
	machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
	const machine_pin_obj_t *pin_obj_p = machine_pin_obj_from_gpio_num(self->gpio_num);
	hal_pwm_stop(pin_obj_p->pwm_ch);
	hal_pinmux_set_function(self->gpio_num, 0);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pwm_deinit_obj, machine_pwm_deinit);




STATIC mp_obj_t machine_pwm_freq(size_t n_args, const mp_obj_t *args){
	machine_pwm_obj_t *self= args[0];
	// -- get frequency 
	if(n_args == 1){
		return MP_OBJ_NEW_SMALL_INT(self->freq);
	}

	// -- set frequency
	const machine_pin_obj_t * pin_obj_p = machine_pin_obj_from_gpio_num(self->gpio_num);
	uint32_t freq = (uint32_t) mp_obj_get_int(args[1]);
	if(HAL_PWM_STATUS_OK != hal_pwm_set_frequency(
				pin_obj_p->pwm_ch, 
				freq, 
				&(self->total_count)
				)){
		mp_raise_ValueError("freq value error!");
	}
	self->freq = freq;

	if(HAL_PWM_STATUS_OK != hal_pwm_set_duty_cycle(
				pin_obj_p->pwm_ch,
				HAL_DUTY_CYCLE_FROM_DUTY_CYCLE_BASE1024(self->duty, self->total_count)
				)){
		nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "hal set duty cycle failed!"));
	}
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pwm_freq_obj, 1, 2, machine_pwm_freq);




STATIC mp_obj_t machine_pwm_duty(size_t n_args, const mp_obj_t *args){
	machine_pwm_obj_t *self = MP_OBJ_TO_PTR(args[0]);

	// -- get duty cycle
	if(n_args == 1){
		return MP_OBJ_NEW_SMALL_INT(self->duty);
	}

	// -- set duty cycle 
	const machine_pin_obj_t *pin_obj_p = machine_pin_obj_from_gpio_num(self->gpio_num);

	uint32_t duty1024 = (uint32_t) mp_obj_get_int(args[1]);
	if(duty1024 > 1024){
		mp_raise_ValueError("pwm duty value out of range!");
	}

	if(HAL_PWM_STATUS_OK != hal_pwm_set_duty_cycle(
				pin_obj_p->pwm_ch, 
				HAL_DUTY_CYCLE_FROM_DUTY_CYCLE_BASE1024(duty1024, self->total_count))
			){
		nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "hal set duty cycle failed!"));
	}
	self->duty = duty1024;

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pwm_duty_obj, 1, 2, machine_pwm_duty);




STATIC const mp_rom_map_elem_t machine_pwm_locals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_pwm_init_obj) },
	{ MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_pwm_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&machine_pwm_freq_obj) },
	{ MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&machine_pwm_duty_obj) },

	// -- Clock Source
	{ MP_ROM_QSTR(MP_QSTR_CLK_SRC_32K), MP_ROM_INT(HAL_PWM_CLOCK_32KHZ) },
	{ MP_ROM_QSTR(MP_QSTR_CLK_SRC_2M), MP_ROM_INT(HAL_PWM_CLOCK_2MHZ) },
	{ MP_ROM_QSTR(MP_QSTR_CLK_SRC_20M), MP_ROM_INT(HAL_PWM_CLOCK_20MHZ) },
	{ MP_ROM_QSTR(MP_QSTR_CLK_SRC_26M), MP_ROM_INT(HAL_PWM_CLOCK_26MHZ) },
	{ MP_ROM_QSTR(MP_QSTR_CLK_SRC_40M), MP_ROM_INT(HAL_PWM_CLOCK_40MHZ) },
	{ MP_ROM_QSTR(MP_QSTR_CLK_SRC_52M), MP_ROM_INT(HAL_PWM_CLOCK_52MHZ) },
};
STATIC MP_DEFINE_CONST_DICT(machine_pwm_locals_dict, machine_pwm_locals_table);

const mp_obj_type_t machine_pwm_type = {
	{ &mp_type_type },
	.name = MP_QSTR_PWM,
	.print = machine_pwm_print,
	.make_new = machine_pwm_make_new,
	.locals_dict = (mp_obj_dict_t*)&machine_pwm_locals_dict,
};
