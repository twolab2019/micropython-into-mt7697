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

/*
 * Pin(pin number, [mode: input(default)/output],[pull: up/down/none(default)])
 * ex: 
 *     p6 = Pin(6, Pin.IN)
 *     print(p6.value())
 *
 *     p7 = Pin(7, Pin.OUT)
 *     p7.value(1)
 *     p7.value(0)
 *     p7.toggle()
 *     p7.on()
 *     p7.off()
 *
 * */

#include <stdio.h>
#include <string.h>

#include "hal_gpio_7687.h"
#include "type_def.h"

#include "hal.h"
#include "system_mt7687.h"
#include "top.h"
#include "bsp_gpio_ept_config.h"

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "extmod/virtpin.h"
#include "machine_pin.h"

STATIC mp_obj_t machine_pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args);
static void machine_pin_isr(void * machine_pin_obj_ptr);

/* Pin Object Table */
const machine_pin_obj_t machine_pin_obj[18] = {
	{{&machine_pin_type}, HAL_GPIO_2 , 0   , HAL_GPIO_2_GPIO2   , HAL_PWM_23 , HAL_GPIO_2_PWM23 , 0                 , HAL_EINT_NUMBER_MAX , HAL_ADC_CHANNEL_MAX },  // P0
	{{&machine_pin_type}, HAL_GPIO_3 , 1   , HAL_GPIO_3_GPIO3   , HAL_PWM_24 , HAL_GPIO_3_PWM24 , HAL_GPIO_3_EINT2  , HAL_EINT_NUMBER_2   , HAL_ADC_CHANNEL_MAX },  // P1
	{{&machine_pin_type}, HAL_GPIO_0 , 2   , HAL_GPIO_0_GPIO0   , HAL_PWM_0  , HAL_GPIO_0_PWM0  , HAL_GPIO_0_EINT0  , HAL_EINT_NUMBER_0   , HAL_ADC_CHANNEL_MAX },  // P2
	{{&machine_pin_type}, HAL_GPIO_39, 3   , HAL_GPIO_39_GPIO39 , HAL_PWM_22 , HAL_GPIO_39_PWM22, HAL_GPIO_39_EINT22, HAL_EINT_NUMBER_22  , HAL_ADC_CHANNEL_MAX }, // P3
	{{&machine_pin_type}, HAL_GPIO_34, 4   , HAL_GPIO_34_GPIO34 , HAL_PWM_35 , HAL_GPIO_34_PWM35, 0                 , HAL_EINT_NUMBER_MAX , HAL_ADC_CHANNEL_MAX }, // P4
	{{&machine_pin_type}, HAL_GPIO_33, 5   , HAL_GPIO_33_GPIO33 , HAL_PWM_34 , HAL_GPIO_33_PWM34, 0                 , HAL_EINT_NUMBER_MAX , HAL_ADC_CHANNEL_MAX }, // P5
	{{&machine_pin_type}, HAL_GPIO_37, 6   , HAL_GPIO_37_GPIO37 , HAL_PWM_20 , HAL_GPIO_37_PWM20, HAL_GPIO_37_EINT20, HAL_EINT_NUMBER_20  , HAL_ADC_CHANNEL_MAX }, // P6
	{{&machine_pin_type}, HAL_GPIO_36, 7   , HAL_GPIO_36_GPIO36 , HAL_PWM_19 , HAL_GPIO_36_PWM19, 0                 , HAL_EINT_NUMBER_MAX , HAL_ADC_CHANNEL_MAX }, // P7
	{{&machine_pin_type}, HAL_GPIO_27, 8   , HAL_GPIO_27_GPIO27 , HAL_PWM_28 , HAL_GPIO_27_PWM28, 0                 , HAL_EINT_NUMBER_MAX , HAL_ADC_CHANNEL_MAX }, // P8
	{{&machine_pin_type}, HAL_GPIO_28, 9   , HAL_GPIO_28_GPIO28 , HAL_PWM_29 , HAL_GPIO_28_PWM29, 0                 , HAL_EINT_NUMBER_MAX , HAL_ADC_CHANNEL_MAX }, // P9
	{{&machine_pin_type}, HAL_GPIO_32, 10  , HAL_GPIO_32_GPIO32 , HAL_PWM_33 , HAL_GPIO_32_PWM33, 0                 , HAL_EINT_NUMBER_MAX , HAL_ADC_CHANNEL_MAX }, // P10
	{{&machine_pin_type}, HAL_GPIO_29, 11  , HAL_GPIO_29_GPIO29 , HAL_PWM_30 , HAL_GPIO_29_PWM30, 0                 , HAL_EINT_NUMBER_MAX , HAL_ADC_CHANNEL_MAX }, // P11
	{{&machine_pin_type}, HAL_GPIO_30, 12  , HAL_GPIO_30_GPIO30 , HAL_PWM_31 , HAL_GPIO_30_PWM31, 0                 , HAL_EINT_NUMBER_MAX , HAL_ADC_CHANNEL_MAX }, // P12
	{{&machine_pin_type}, HAL_GPIO_31, 13  , HAL_GPIO_31_GPIO31 , HAL_PWM_32 , HAL_GPIO_31_PWM32, 0                 , HAL_EINT_NUMBER_MAX , HAL_ADC_CHANNEL_MAX }, // P13
	{{&machine_pin_type}, HAL_GPIO_57, 14  , HAL_GPIO_57_GPIO57 , HAL_PWM_36 , HAL_GPIO_57_PWM36, 0                 , HAL_EINT_NUMBER_MAX , HAL_ADC_CHANNEL_0   }, // P14
	{{&machine_pin_type}, HAL_GPIO_58, 15  , HAL_GPIO_58_GPIO58 , HAL_PWM_37 , HAL_GPIO_58_PWM37, 0                 , HAL_EINT_NUMBER_MAX , HAL_ADC_CHANNEL_1   }, // P15
	{{&machine_pin_type}, HAL_GPIO_59, 16  , HAL_GPIO_59_GPIO59 , HAL_PWM_38 , HAL_GPIO_59_PWM38, 0                 , HAL_EINT_NUMBER_MAX , HAL_ADC_CHANNEL_2   }, // P16
	{{&machine_pin_type}, HAL_GPIO_60, 17  , HAL_GPIO_60_GPIO60 , HAL_PWM_39 , HAL_GPIO_60_PWM39, 0                 , HAL_EINT_NUMBER_MAX , HAL_ADC_CHANNEL_3   }, // P17
};



void machine_pins_init(void){
	/* depending on MTK BSP tools */ 
	bsp_ept_gpio_setting_init();
}

void machine_pins_deinit(void){
	// TODO
}


const machine_pin_obj_t * machine_pin_obj_from_gpio_num(hal_gpio_pin_t gpio_num){
	int pin_obj_index = 0;
	int pin_obj_nums = sizeof(machine_pin_obj)/sizeof(machine_pin_obj_t);
	for(pin_obj_index = 0; pin_obj_index < pin_obj_nums; pin_obj_index++){
		if(machine_pin_obj[pin_obj_index].gpio_num == gpio_num){
			return machine_pin_obj + pin_obj_index;
		}
	}
	return NULL;
}

const machine_pin_obj_t * machine_pin_obj_from_pin_num(uint8_t pin_num){
	int pin_obj_index = 0;
	int pin_obj_nums = sizeof(machine_pin_obj)/sizeof(machine_pin_obj_t);
	for(pin_obj_index = 0; pin_obj_index < pin_obj_nums; pin_obj_index++){
		if(machine_pin_obj[pin_obj_index].pin_num == pin_num){
			return machine_pin_obj + pin_obj_index;
		}
	}
	return NULL;
}

/*
 * */
const machine_pin_obj_t * machine_pin_obj_from_upy_obj(mp_obj_t upy_obj){
	if(MP_OBJ_IS_INT(upy_obj)){
		return machine_pin_obj_from_pin_num(mp_obj_get_int(upy_obj));
	}else if (MP_OBJ_IS_TYPE(upy_obj,&machine_pin_type)){
		return (machine_pin_obj_t *) upy_obj;
	}
	return mp_const_none;
}


hal_gpio_pin_t machine_pin_get_id(mp_obj_t pin_in){
	if(mp_obj_get_type(pin_in) != &machine_pin_type){
		mp_raise_ValueError("expecting a pin");
	}
	const machine_pin_obj_t *self = pin_in;
	return self->gpio_num;
}


STATIC void machine_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kink){
	const machine_pin_obj_t *self = self_in;
	mp_printf(print, "Pin(%u)", self->pin_num);
}


/*
 * Pin(7,Pin.IN, Pin.PULL_UP)
 * */
STATIC mp_obj_t machine_pin_obj_init_helper(const machine_pin_obj_t *self, size_t n_args, const mp_obj_t * pos_args, mp_map_t *kw_args){
	enum {ARG_mode, ARG_pull, ARG_value};
	static const mp_arg_t allowed_args[] = {
		// {MP_QSTR_mode, MP_ARG_REQUIRED | MP_ARG_INT , {.u_int = -1}},
		{MP_QSTR_mode, MP_ARG_INT , {.u_int = -1}},
		{MP_QSTR_pull, MP_ARG_OBJ, {.u_obj = mp_const_none} },
		{MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	/* get io mode */ 
	uint mode = args[ARG_mode].u_int;
	if(mode == -1){
		mode = GPIO_PIN_DIR_INPUT;
	}else if(!(mode == GPIO_PIN_DIR_INPUT || mode == GPIO_PIN_DIR_OUTPUT)){
		mp_raise_ValueError("invalid pin mode value");
		return mp_const_none;
	}

	/* get pull mode */ 
	uint pull = GPIO_PIN_PULL_DISABLE;
	if(args[1].u_obj != mp_const_none){
		pull = mp_obj_get_int(args[1].u_obj);
	}
	if(!(pull == GPIO_PIN_PULL_DOWN || pull == GPIO_PIN_PULL_UP || pull == GPIO_PIN_PULL_DISABLE)){
		mp_raise_ValueError("invalid pin mode value");
	}
	if(HAL_GPIO_STATUS_OK != hal_gpio_init(self->gpio_num)){
		mp_raise_msg(&mp_type_OSError, "hal gpio init error");
		return mp_const_none;
	}

	if(HAL_PINMUX_STATUS_OK != hal_pinmux_set_function(self->gpio_num, self->gpio_pinmux_num)){
		mp_raise_msg(&mp_type_OSError, "hal pinmux error");
		return mp_const_none;
	}

	/* Set direction */ 
	hal_gpio_status_t res;
	switch(mode){
		case GPIO_PIN_DIR_OUTPUT:
			res = hal_gpio_set_direction(self->gpio_num, HAL_GPIO_DIRECTION_OUTPUT);
			break;
		case GPIO_PIN_DIR_INPUT:
			res = hal_gpio_set_direction(self->gpio_num, HAL_GPIO_DIRECTION_INPUT);
			break;
		default:
			mp_raise_ValueError("invalid gpio direction");
			return mp_const_none;
			break;
	}

	if(HAL_GPIO_STATUS_OK != res){
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,"hal gpio set dir error" ));
		return mp_const_none;
	}

	/* Set pull */ 
	if(mode == GPIO_PIN_DIR_INPUT){
		switch(pull){
			case GPIO_PIN_PULL_UP:
				res = hal_gpio_pull_up(self->gpio_num);
				break;
			case GPIO_PIN_PULL_DOWN:
				res = hal_gpio_pull_down(self->gpio_num);
				break;
			case GPIO_PIN_PULL_DISABLE:
				res = hal_gpio_disable_pull(self->gpio_num);
				break;
			default:
				nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError,"invalid pull mode value"));
				return mp_const_none;
				break;
		}
	}else{
		res = hal_gpio_disable_pull(self->gpio_num);
	}

	if(HAL_GPIO_STATUS_OK != res){
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError,"hal gpio pull up/down/disable error"));
		return mp_const_none;
	}

	return mp_const_none;
}


mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
	mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);


	/* get args[0] : id : pin_num */
	uint pin_num = mp_obj_get_int(args[0]);
	const machine_pin_obj_t *self = machine_pin_obj_from_pin_num(pin_num);

	if(self == NULL || self->base.type == NULL){
		mp_raise_ValueError("invalid pin");
	}

	if(n_args >= 1 || n_kw > 0) {
		mp_map_t kw_args;
		mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
		machine_pin_obj_init_helper(self, n_args - 1, args + 1, &kw_args);
	}else{
		mp_raise_ValueError("invalid argument");
	}

	return MP_OBJ_FROM_PTR(self);
}


STATIC mp_obj_t machine_pin_on(mp_obj_t self_in){
	const machine_pin_obj_t *self = self_in;
	// hal_gpio_set_output(self->gpio_num, HAL_GPIO_DATA_HIGH);
	// mp_hal_pin_write(self,1);
	_mp_hal_pin_write(self->gpio_num,1);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_on_obj, machine_pin_on);

STATIC mp_obj_t machine_pin_toggle(mp_obj_t self_in){
	const machine_pin_obj_t *self = self_in;
	// hal_gpio_set_output(self->gpio_num, HAL_GPIO_DATA_HIGH);
	hal_gpio_toggle_pin(self->gpio_num);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_toggle_obj, machine_pin_toggle);


STATIC mp_obj_t machine_pin_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args){
	return machine_pin_obj_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_init_obj, 1, machine_pin_init);


STATIC mp_obj_t machine_pin_value(size_t n_args, const mp_obj_t *args){
	return machine_pin_call(args[0], n_args - 1, 0, args + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_value_obj, 1, 2, machine_pin_value);


STATIC mp_obj_t machine_pin_off(mp_obj_t self_in){
	const machine_pin_obj_t *self = self_in;
	// hal_gpio_set_output(self->gpio_num, HAL_GPIO_DATA_LOW);
	mp_hal_pin_write(self,0);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_off_obj, machine_pin_off);



/*
 * ex:
 *     p6 = Pin(6)
 *     p6.irq(lambda x: print('hello')) 
 *     p6.irq(lambda x: print('hello'), Pin.IRQ_RISING) 
 *     p6.irq(lambda x: print('hello'), Pin.IRQ_RISING, 50) 
 * */
STATIC mp_obj_t machine_pin_irq(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args){
	enum {ARG_handler, ARG_trigger, ARG_debounce};
	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_handler , MP_ARG_OBJ, {.u_obj = mp_const_none }},
		{ MP_QSTR_trigger , MP_ARG_INT, {.u_int = HAL_EINT_EDGE_RISING}},
		{ MP_QSTR_debounce, MP_ARG_INT, {.u_int = 0 }},
	};
	const machine_pin_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	mp_arg_val_t _args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all(n_args -1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, _args);

	if(n_args > 1){
		mp_obj_t handler = _args[ARG_handler].u_obj;
		uint32_t trigger = _args[ARG_trigger].u_int;
		uint32_t debounce = _args[ARG_debounce].u_int;
		hal_gpio_init(self->gpio_num);
		hal_pinmux_set_function( self->gpio_num, self->eint_pinmux_num);
		hal_eint_config_t eint_config;
		eint_config.trigger_mode = trigger;
		eint_config.debounce_time = debounce;
		hal_eint_init(self->eint_num, &eint_config);
		MP_STATE_PORT(machine_irq_callback)[self->pin_num] = handler;
		hal_eint_register_callback(self->eint_num, machine_pin_isr, (void*)self);
	}
	return mp_const_none;
pin_irq_fail:
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_irq_obj, 1, machine_pin_irq);


uint8_t _mp_hal_pin_read(hal_gpio_pin_t gpio_num){
	hal_gpio_data_t val;
	hal_gpio_set_direction(gpio_num, HAL_GPIO_DIRECTION_INPUT);
	hal_gpio_get_input(gpio_num, &val);
	return (val == HAL_GPIO_DATA_LOW)?(0):(1);
}


STATIC mp_obj_t machine_pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args){
	mp_arg_check_num(n_args, n_kw, 0, 1, false);
	const machine_pin_obj_t *self = self_in;
	if(n_args == 0){
		hal_gpio_data_t val;
		hal_gpio_get_input(self->gpio_num, &val);
		return (mp_obj_t) MP_OBJ_NEW_SMALL_INT(val);
	} else {
		hal_gpio_set_output(self->gpio_num, mp_obj_is_true(args[0]) ? (HAL_GPIO_DATA_HIGH):(HAL_GPIO_DATA_LOW));
		return (mp_obj_t) mp_const_none;
	}
}

STATIC mp_uint_t pin_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode){
	(void)errcode;
	const machine_pin_obj_t * self = self_in;
	hal_gpio_data_t val;
	switch(request){
		case MP_PIN_READ:
			hal_gpio_get_input(self->gpio_num, &val);
			// return MP_OBJ_NEW_SMALL_INT(val);
			return val;
			break;
		case MP_PIN_WRITE:
			hal_gpio_set_output(self->gpio_num, (arg==0)?(HAL_GPIO_DATA_LOW):(HAL_GPIO_DATA_HIGH));
			return 0;
			break;
		default:
			return -1;
	}
	return -1;
}

STATIC const mp_rom_map_elem_t machine_pin_locals_dict_table[] = {
	/* instance methods */
	{MP_ROM_QSTR(MP_QSTR_init),         MP_ROM_PTR(&machine_pin_init_obj)},
	{MP_ROM_QSTR(MP_QSTR_value),        MP_ROM_PTR(&machine_pin_value_obj)},
	{MP_ROM_QSTR(MP_QSTR_off),          MP_ROM_PTR(&machine_pin_off_obj)},
	{MP_ROM_QSTR(MP_QSTR_on),           MP_ROM_PTR(&machine_pin_on_obj)},
	{MP_ROM_QSTR(MP_QSTR_toggle),       MP_ROM_PTR(&machine_pin_toggle_obj)},
	{MP_ROM_QSTR(MP_QSTR_irq),          MP_ROM_PTR(&machine_pin_irq_obj)},

	{ MP_ROM_QSTR(MP_QSTR_IN),          MP_ROM_INT(GPIO_PIN_DIR_INPUT) },
	{ MP_ROM_QSTR(MP_QSTR_OUT),         MP_ROM_INT(GPIO_PIN_DIR_OUTPUT) },

	/* class constants */
	{ MP_ROM_QSTR(MP_QSTR_PULL_UP),     MP_ROM_INT(GPIO_PIN_PULL_UP) },
	{ MP_ROM_QSTR(MP_QSTR_PULL_DOWN),   MP_ROM_INT(GPIO_PIN_PULL_DOWN) },
	{ MP_ROM_QSTR(MP_QSTR_PULL_DISABLE),MP_ROM_INT(GPIO_PIN_PULL_DISABLE) },

	{ MP_ROM_QSTR(MP_QSTR_IRQ_RISING),  MP_ROM_INT(HAL_EINT_EDGE_RISING) },
	{ MP_ROM_QSTR(MP_QSTR_IRQ_FALLING), MP_ROM_INT(HAL_EINT_EDGE_FALLING) },
	{ MP_ROM_QSTR(MP_QSTR_IRQ_FALLING_AND_RISING), MP_ROM_INT(HAL_EINT_EDGE_FALLING_AND_RISING) },
	{ MP_ROM_QSTR(MP_QSTR_IRQ_HIGH),    MP_ROM_INT(HAL_EINT_LEVEL_HIGH) },
	{ MP_ROM_QSTR(MP_QSTR_IRQ_LOW),     MP_ROM_INT(HAL_EINT_LEVEL_LOW) },

};
STATIC MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);



STATIC const mp_pin_p_t pin_pin_p = {
	.ioctl = pin_ioctl,
};

const mp_obj_type_t machine_pin_type = {
	{ &mp_type_type },
	.name = MP_QSTR_Pin,
	.print = machine_pin_print, 
	.make_new = mp_pin_make_new,
	.call = machine_pin_call,
	.protocol = &pin_pin_p,
	.locals_dict = (mp_obj_t)&machine_pin_locals_dict,
};


/*
 * callback for mtk hal
 * */
static void machine_pin_isr(void * machine_pin_obj_ptr){
	const machine_pin_obj_t * self = (const machine_pin_obj_t *)machine_pin_obj_ptr;
	mp_obj_t cb = MP_STATE_PORT(machine_irq_callback)[self->pin_num];
	mp_sched_schedule(cb, MP_OBJ_FROM_PTR(self));
}

int _mp_hal_pin_write(uint32_t gpio_num, uint8_t val){
	switch(0xffff & (gpio_num >> 5) ){
		case 0:
			if(val != 0){
				DRV_WriteReg32(IOT_GPIO_AON_BASE + IOT_GPIO_DOUT1_SET, 1 << (gpio_num & 0x0000001f));
			}else{
				DRV_WriteReg32(IOT_GPIO_AON_BASE + IOT_GPIO_DOUT1_RESET, 1 << (gpio_num & 0x0000001f));
			}
			break;
		case 1:
			if(val != 0){
				DRV_WriteReg32(IOT_GPIO_AON_BASE + IOT_GPIO_DOUT2_SET, 1 << (gpio_num & 0x0000001f));
			}else{
				DRV_WriteReg32(IOT_GPIO_AON_BASE + IOT_GPIO_DOUT2_RESET, 1 << (gpio_num & 0x0000001f));
			}
			break;
		default:
			return -1;
	}
	return 0;
}
