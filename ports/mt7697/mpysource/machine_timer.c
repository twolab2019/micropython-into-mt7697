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
#include "py/obj.h"
#include "py/runtime.h"
#include "modmachine.h"
#include "mphalport.h"
#include "machine_timer.h"

typedef struct _machine_timer_obj_t {
    mp_obj_base_t        base;
    const uint8_t        id;
	const hal_gpt_port_t port;
	uint32_t             period_ms;
	hal_gpt_timer_type_t mode;
	mp_obj_t             callback;
} machine_timer_obj_t;

const mp_obj_type_t machine_timer_type;


/**
 * HAL_GPT_0 : tickless timer, clock source is 32kHz
 *
 * HAL_GPT_1 : user defined, clock source is 32kHz
 *
 * ---------------------------------------------------------
 * HAL_GPT_2 : Use to set a millisecond delay 
 *             and get 1/32Khz free count, clock source is 32kHz
 *
 * HAL_GPT_4 : Use to set a microsecond delay 
 *             and get microsecond free count, clock source is bus_clock
 */
machine_timer_obj_t machine_timer_obj[] = {
	{ {&machine_timer_type}, 0, HAL_GPT_0, 0, HAL_GPT_TIMER_TYPE_ONE_SHOT, mp_const_none },
	{ {&machine_timer_type}, 1, HAL_GPT_1, 0, HAL_GPT_TIMER_TYPE_ONE_SHOT, mp_const_none },
	{ {&machine_timer_type}, 2, HAL_GPT_2, 0, HAL_GPT_TIMER_TYPE_ONE_SHOT, mp_const_none },
	{ {&machine_timer_type}, 3, HAL_GPT_4, 0, HAL_GPT_TIMER_TYPE_ONE_SHOT, mp_const_none },
};

machine_timer_obj_t * machine_timer_obj_from_id(mp_int_t id){
	if(id >= 0 && id < 2){ // only support timer 0 and 1
		return &machine_timer_obj[id];
	}
	return NULL;
}

STATIC void machine_timer_isr(void *self_in) {
    machine_timer_obj_t *self = self_in;
	if(mp_obj_is_callable(self->callback)){
    	mp_sched_schedule(self->callback, self);
	}
    // mp_hal_wake_main_task_from_isr();
}


STATIC mp_obj_t machine_timer_init_helper(machine_timer_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
        ARG_mode,
        ARG_callback,
        ARG_period,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,      MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_GPT_TIMER_TYPE_ONE_SHOT } },
        { MP_QSTR_callback,  MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_period_ms, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	mp_int_t _period_ms = args[ARG_period].u_int;
    if( _period_ms > 0){
		self->period_ms = _period_ms;
	}
	
	if(self->period_ms == 0){
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,"invalid value period_ms: %d", _period_ms));
	}

    self->mode = args[ARG_mode].u_int;
	mp_obj_t callback_obj = args[ARG_callback].u_obj;
	if(callback_obj == mp_const_none || mp_obj_is_callable(callback_obj)){
    	self->callback = callback_obj;
	}else{
		mp_raise_ValueError("callback must be None or a callable object");
	}

	hal_gpt_deinit(self->port);
	hal_gpt_init(self->port);
	hal_gpt_register_callback(self->port, machine_timer_isr, (void*)self);
	hal_gpt_start_timer_ms(self->port, self->period_ms, self->mode);
    return mp_const_none;
}

STATIC mp_obj_t machine_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // mp_arg_check_num(n_args, n_kw, 1, 1, false);
    machine_timer_obj_t *self = mp_const_none;
	// int i = 0;
	mp_int_t _id = mp_obj_get_int(args[0]);
	// fetch the existed timer by id: 0~3
	self = machine_timer_obj_from_id(_id);
	if(self == NULL){
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,"invalid value: id=%d", _id));
		return mp_const_none;
	}

	mp_map_t kw_args;
	mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
	machine_timer_init_helper(self, n_args - 1, args + 1, &kw_args);
    return MP_OBJ_FROM_PTR(self);
}

/*
 * ex: 
 *     timer.init(mode=Timer.PERIODIC, callback=lambad x: print('hello'), period=1000)
 * */
STATIC mp_obj_t machine_timer_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    machine_timer_init_helper(args[0], n_args - 1, args + 1, kw_args);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_timer_init_obj, 1, machine_timer_init);

STATIC mp_obj_t machine_timer_deinit(mp_obj_t self_in) {
	machine_timer_obj_t *self = self_in;
	hal_gpt_stop_timer(self->port);
	hal_gpt_deinit(self->port);
	self->callback = mp_const_none; // shoud it ?
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_deinit_obj, machine_timer_deinit);

STATIC mp_obj_t machine_timer_stop(mp_obj_t self_in){
	machine_timer_obj_t *self = self_in;
	hal_gpt_status_t res = hal_gpt_stop_timer(self->port);
	if(res != HAL_GPT_STATUS_OK ){
		mp_raise_msg(&mp_type_OSError, "hal stop timer failed!");
	}
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_stop_obj, machine_timer_stop);

STATIC mp_obj_t machine_timer_callback(size_t n_args, const mp_obj_t * args){
    machine_timer_obj_t *self = args[0];
	if(n_args == 1){
		return self->callback;
	}else {
		mp_obj_t callback_obj = args[1];
		if(callback_obj == mp_const_none || mp_obj_is_callable(callback_obj)){
			self->callback = callback_obj;
		}else{
			mp_raise_ValueError("callback must be None or a callable object");
		}
	}
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_timer_callback_obj,1,2,machine_timer_callback);

STATIC void machine_timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_timer_obj_t *self = self_in;

    mp_printf(print, "Timer(%u; ", self->id);
    mp_printf(print, "period=%u ms, ", self->period_ms);
    mp_printf(print, "mode=%s, ", (self->mode == HAL_GPT_TIMER_TYPE_ONE_SHOT)?("ONE SHOT"):("REPEAT"));
	hal_gpt_running_status_t running_status;
	hal_gpt_get_running_status(self->port, &running_status);
	mp_printf(print, "status=%s)", (running_status == HAL_GPT_STOPPED)?("STOPPED"):("RUNNING") );
}

STATIC const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_timer_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&machine_timer_stop_obj) },
    // { MP_ROM_QSTR(MP_QSTR_counter), MP_ROM_PTR(&machine_timer_counter_obj) },
    { MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&machine_timer_callback_obj) },
    { MP_ROM_QSTR(MP_QSTR_ONE_SHOT), MP_ROM_INT(HAL_GPT_TIMER_TYPE_ONE_SHOT) },
    { MP_ROM_QSTR(MP_QSTR_PERIODIC), MP_ROM_INT(HAL_GPT_TIMER_TYPE_REPEAT) },
};
STATIC MP_DEFINE_CONST_DICT(machine_timer_locals_dict, machine_timer_locals_dict_table);

const mp_obj_type_t machine_timer_type = {
    { &mp_type_type },
    .name = MP_QSTR_Timer,
    .print = machine_timer_print,
    .make_new = machine_timer_make_new,
    .locals_dict = (mp_obj_t)&machine_timer_locals_dict,
};
