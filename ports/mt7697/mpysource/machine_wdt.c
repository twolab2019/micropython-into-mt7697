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

#include "stdio.h"
#include "stdlib.h"
#include "hal.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "modmachine.h"
#include "machine_wdt.h"
/*
 * wdt = WDT(id=0,timeout=5)
 *
 * parameters:
 *
 *     id = 0 only one watch supported
 *     timeout = 0 ~ 30 (unit: second)
 *
 * Usage:
 *     import machine
 *     wdt = machine.WDT()
 *     wdt.init() # watch dog start
 *     wdt.feed() # feed the watch dog 
 *     wdt.deinit() # stop the watch dog
 *
 * */

typedef struct _machine_wdt_obj_t {
	mp_obj_base_t base;
	uint8_t id;
	uint32_t timeout_seconds;
}machine_wdt_obj_t;

STATIC mp_obj_t machine_wdt_feed(mp_obj_t self_in){
	if(HAL_WDT_STATUS_OK != hal_wdt_feed(HAL_WDT_FEED_MAGIC)){
		mp_raise_msg(&mp_type_OSError, "watch feed error!");
	}
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_feed_obj, machine_wdt_feed);

STATIC mp_obj_t machine_wdt_init(mp_obj_t self_in){
	machine_wdt_obj_t * self = MP_OBJ_FROM_PTR(self_in);
	hal_wdt_config_t wdt_config;
	wdt_config.mode = HAL_WDT_MODE_RESET;
	wdt_config.seconds = self->timeout_seconds;

	if(HAL_WDT_STATUS_OK != hal_wdt_disable(HAL_WDT_DISABLE_MAGIC)){
		mp_raise_msg(&mp_type_OSError, "watch dog diable failed!");
	}

	if(HAL_WDT_STATUS_OK != hal_wdt_deinit()){
		mp_raise_msg(&mp_type_OSError, "watch dog deinit failed!");
	}

	if(HAL_WDT_STATUS_OK != hal_wdt_init(&wdt_config)){
		mp_raise_msg(&mp_type_OSError, "watch dog init failed!");
	}

	if(HAL_WDT_STATUS_OK != hal_wdt_enable(HAL_WDT_ENABLE_MAGIC)){
		mp_raise_msg(&mp_type_OSError, "watch dog enable failed!");
	}
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_init_obj, machine_wdt_init);

STATIC mp_obj_t machine_wdt_deinit(mp_obj_t self_in){

	if(HAL_WDT_STATUS_OK != hal_wdt_disable(HAL_WDT_DISABLE_MAGIC)){
		mp_raise_msg(&mp_type_OSError, "watch dog disable failed!");
	}

	if(HAL_WDT_STATUS_OK != hal_wdt_deinit()){
		mp_raise_msg(&mp_type_OSError, "watch dog deinit failed!");
	}

	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_deinit_obj,machine_wdt_deinit);

STATIC machine_wdt_obj_t machine_wdt_obj = {{&machine_wdt_type}, 0, 0};

static mp_obj_t machine_wdt_make_new(const mp_obj_type_t * type, size_t n_args, size_t n_kw, const mp_obj_t * all_args){
	enum {ARG_id, ARG_timeout};
	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_id,      MP_ARG_INT, {.u_int = 0}},
		{ MP_QSTR_timeout, MP_ARG_INT, {.u_int = 5}},
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	/* get id */
	mp_int_t id = args[ARG_id].u_int;
	if(id != 0){
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "WDT(%d) doesn't exist", id));
	}

	/* get timeout_ms */
	mp_int_t timeout_seconds = args[ARG_timeout].u_int;
	if((timeout_seconds < 0) || (timeout_seconds > 30)){
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "timeout %d invalid (should be 0~30 seconds)", timeout_seconds));
	}else if (timeout_seconds == 0){
		hal_wdt_software_reset();
	}

	machine_wdt_obj_t * self = &machine_wdt_obj;
	self->timeout_seconds = timeout_seconds;

	hal_wdt_config_t wdt_config;
	wdt_config.mode = HAL_WDT_MODE_RESET;
	wdt_config.seconds = self->timeout_seconds;
	if(HAL_WDT_STATUS_OK != hal_wdt_init(&wdt_config)){
		mp_raise_msg(&mp_type_OSError, "watch dog init failed!");
	}

	return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t machine_wdt_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_feed),   MP_ROM_PTR(&machine_wdt_feed_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_wdt_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_wdt_deinit_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_wdt_locals_dict, machine_wdt_locals_dict_table);

const mp_obj_type_t machine_wdt_type = {
	{ &mp_type_type },
    .name = MP_QSTR_WDT,
    .make_new = machine_wdt_make_new,
    .locals_dict = (mp_obj_t)&machine_wdt_locals_dict,
};
