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
 * PWM(pin_no, freq, *, duty=0,clk_src=PWM.CLK_SRC_20M)
 *
 * parameters: 
 *    freq : frequency (Hz)
 *           clk src : min freq ~ 2048 base freq (max)
 *           -----------------------------
 *           2M Hz   : 31 Hz  ~ 1900 Hz
 *           20M Hz  : 305 Hz ~ 19000 Hz
 *           40M Hz  : 610 Hz ~ 39000 Hz 
 *    duty : duty cycle : 0 ~ 1024
 * 
 * example code:
 *
 * >>> from machine import PWM
 * >>> pwm7 = PWM(7)  
 * >>> pwm7.duty() # get duty cycle 
 * >>> pwm7.freq() # get frequency 
 * >>> pwm7.duty(50) # set duty cycle
 * >>> pwm7.freq(400) # set frequency
 * >>> pwm7.deinit() # disable PWM 
 * */

typedef struct _machine_pwm_obj_t{
	mp_obj_base_t base;
	hal_gpio_pin_t gpio_num;
	uint32_t duty;
	uint32_t freq;
	uint32_t get_total_count;
}machine_pwm_obj_t;

static int hal_clk_const_set = -1;

uint32_t clk_from_hal_const(int constant){
	switch(constant){
// 		case HAL_PWM_CLOCK_32KHZ: 
// 			return 32768;
		case HAL_PWM_CLOCK_2MHZ: 
			return 2000000;
		case HAL_PWM_CLOCK_20MHZ: 
			return 20000000;
// 		case HAL_PWM_CLOCK_26MHZ: 
// 			return 26000000;
		case HAL_PWM_CLOCK_40MHZ: 
			return 40000000;
// 		case HAL_PWM_CLOCK_52MHZ: 
// 			return 52000000;
		default : 
			return 0;
	}
}


static int _pwm_set_freq(hal_pwm_channel_t hal_pwm_ch, uint32_t* freq_p, uint32_t * get_total_count_ptr){

	// -- check total count is acceptable 
	uint32_t clk_src = clk_from_hal_const(hal_clk_const_set);
	uint32_t cal_total_count = clk_src / *freq_p;
	if(clk_src == 0){
		return -1;
	}
	// -- total count fix for 20M
	uint32_t FIX_20MHZ_FACTOR = 1;
	if(hal_clk_const_set == (int) HAL_PWM_CLOCK_20MHZ){
		FIX_20MHZ_FACTOR = 2;
	}

	cal_total_count *= FIX_20MHZ_FACTOR;

	if(cal_total_count >= 1024 && cal_total_count <= 0xffff){
	}else if(cal_total_count < 1024){
		*freq_p = clk_src / 1024 * FIX_20MHZ_FACTOR; 
	}else if (cal_total_count > 0xffff){
		*freq_p = clk_src / 0xffff * FIX_20MHZ_FACTOR;
	}

	// -- pwm set freq
	if(HAL_PWM_STATUS_OK != hal_pwm_set_frequency(hal_pwm_ch, *freq_p, get_total_count_ptr) ){
		return -2;
	}
	// -- total count fix for 20M
	*get_total_count_ptr *= FIX_20MHZ_FACTOR;

	return 0;
}

static int _pwm_set_duty(hal_pwm_channel_t hal_pwm_ch, uint32_t duty_1024, uint32_t total_count){
	duty_1024 = (duty_1024 > 1024)?(1024):(duty_1024);

	uint32_t hal_duty_count = ( duty_1024 * total_count ) / 1024;


	if(HAL_PWM_STATUS_OK != hal_pwm_set_duty_cycle(hal_pwm_ch, hal_duty_count)){
		return -1;
	}
	return 0;
}

/*
 * */
STATIC void machine_pwm_init_helper(machine_pwm_obj_t * self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args){

	enum {ARG_freq, ARG_duty, ARG_clk_src};
	static const mp_arg_t allowed_args[] = {
		{MP_QSTR_freq,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = -1} },
		{MP_QSTR_duty,    MP_ARG_INT,                   {.u_int = 0 } },
		{MP_QSTR_clk_src, MP_ARG_INT,                   {.u_int = HAL_PWM_CLOCK_20MHZ } },
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	// -- fetch arguments 
	uint32_t freq = (uint32_t) args[ARG_freq].u_int;
	int duty = args[ARG_duty].u_int;
	int hal_clk_src_const = args[ARG_clk_src].u_int;


	// -- fetch pin obj from gpio_num
	const machine_pin_obj_t *pin_obj_p = machine_pin_obj_from_gpio_num(self->gpio_num);
	if(pin_obj_p == 0){
		mp_raise_ValueError("pin num error!");
		return;
	}

	// -- pin GPIO init
	if(HAL_GPIO_STATUS_OK != hal_gpio_init(pin_obj_p->gpio_num)){
		nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "hal init fail!"));
		return;
	}

	// -- pin pinmux set
	if(HAL_PINMUX_STATUS_OK != hal_pinmux_set_function(pin_obj_p->gpio_num, pin_obj_p->pwm_pinmux_num)){
		nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "hal pinmux fail!"));
		return;
	}

	// -- check hal_clk_src_const
	if( clk_from_hal_const(hal_clk_src_const) == 0)
	if(hal_clk_src_const < 0 || hal_clk_src_const >= HAL_PWM_CLOCK_52MHZ){
		mp_raise_ValueError("pwm clk src set error!!");
	}

	// -- pwm init and set clk src
	if(hal_clk_const_set == -1){
		if(HAL_PWM_STATUS_OK != hal_pwm_init(hal_clk_src_const)){
			nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "hal set clk source fail!"));
			return;
		}
		// -- set clk src ok
		hal_clk_const_set = hal_clk_src_const;
	}else if(hal_clk_const_set == hal_clk_src_const){
		// -- do nothing
	}else{
		mp_raise_ValueError("clk src set error!");
	}

	// -- pwm set freq
	if(_pwm_set_freq(pin_obj_p->pwm_ch, &freq, &(self->get_total_count)) < 0){
		mp_raise_ValueError("pwm set freq error!!");
	}
	self->freq = freq;

	// -- pwm set duty
	if(_pwm_set_duty(pin_obj_p->pwm_ch, duty, self->get_total_count) < 0){
		self->freq = 0;
		self->duty = 0;
		mp_raise_ValueError("pwm duty value out of range!");
		return;
	}
	self->duty = (duty > 1024)?(1024):(duty);

	// -- pwm start 
	if(HAL_PWM_STATUS_OK != hal_pwm_start(pin_obj_p->pwm_ch)){
		self->freq = 0;
		self->duty = 0;
		nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "hal pwm start fail!"));
		return;
	}

	return;
}

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
	if(self->get_total_count != 0){
		mp_printf(print, ", freq=%u, duty=%u, clk src=%lu", 
				freq, 
				duty_b1024,
			   	clk_from_hal_const(hal_clk_const_set));
	}
	mp_printf(print, ")");

}

STATIC mp_obj_t machine_pwm_deinit(mp_obj_t self_in){
	machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
	const machine_pin_obj_t *pin_obj_p = machine_pin_obj_from_gpio_num(self->gpio_num);
	// if(hal_pwm_deinit() != HAL_PWM_STATUS_OK){
	if(hal_pwm_stop(pin_obj_p->pwm_ch) != HAL_PWM_STATUS_OK){
		nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "pwm deinit fail!"));
	}
	// hal_clk_const_set = -1;
	hal_pinmux_set_function(self->gpio_num, 0);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pwm_deinit_obj, machine_pwm_deinit);


STATIC mp_obj_t machine_pwm_freq(size_t n_args, const mp_obj_t *args){
	machine_pwm_obj_t *self= args[0];
	if(n_args == 1){
		return MP_OBJ_NEW_SMALL_INT(self->freq);
	}

	const machine_pin_obj_t * pin_obj_p = machine_pin_obj_from_gpio_num(self->gpio_num);
	uint32_t freq = (uint32_t) mp_obj_get_int(args[1]);

	// -- set freq
	if(_pwm_set_freq(pin_obj_p->pwm_ch, &freq, &(self->get_total_count)) < 0){
		mp_raise_ValueError("invalid freq value!");
	}
	self->freq = freq;

	// -- restore the duty cycle
	if(_pwm_set_duty(pin_obj_p->pwm_ch, self->duty, self->get_total_count) < 0){
		mp_raise_ValueError("restore duty error!");
	}
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pwm_freq_obj, 1, 2, machine_pwm_freq);


/*
 * args[0] : self
 * args[1] : duty_1024
 * */
STATIC mp_obj_t machine_pwm_duty(size_t n_args, const mp_obj_t *args){
	machine_pwm_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	const machine_pin_obj_t *pin_obj_p = machine_pin_obj_from_gpio_num(self->gpio_num);

	if(n_args == 1){
		return MP_OBJ_NEW_SMALL_INT(self->duty);
	}

	// -- pwm set duty
	uint32_t duty_1024 = (uint32_t) mp_obj_get_int(args[1]);
	if(_pwm_set_duty(pin_obj_p->pwm_ch, duty_1024, self->get_total_count) < 0){
		mp_raise_ValueError("set pwm duty!");
	}
	self->duty = (duty_1024>1024)?(1024):(duty_1024);

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pwm_duty_obj, 1, 2, machine_pwm_duty);




STATIC const mp_rom_map_elem_t machine_pwm_locals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_pwm_init_obj) },
	{ MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_pwm_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&machine_pwm_freq_obj) },
	{ MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&machine_pwm_duty_obj) },

	// -- clock source
	{ MP_ROM_QSTR(MP_QSTR_CLK_SRC_2M), MP_ROM_INT(HAL_PWM_CLOCK_2MHZ) },
	{ MP_ROM_QSTR(MP_QSTR_CLK_SRC_20M), MP_ROM_INT(HAL_PWM_CLOCK_20MHZ) },
	{ MP_ROM_QSTR(MP_QSTR_CLK_SRC_40M), MP_ROM_INT(HAL_PWM_CLOCK_40MHZ) },
};
STATIC MP_DEFINE_CONST_DICT(machine_pwm_locals_dict, machine_pwm_locals_table);

const mp_obj_type_t machine_pwm_type = {
	{ &mp_type_type },
	.name = MP_QSTR_PWM,
	.print = machine_pwm_print,
	.make_new = machine_pwm_make_new,
	.locals_dict = (mp_obj_dict_t*)&machine_pwm_locals_dict,
};
