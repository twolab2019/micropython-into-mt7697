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

#include <stdlib.h>
#include <stdio.h>

#include "hal.h"

#include "py/obj.h"
#include "py/runtime.h"
#include "modmachine.h"

#include "machine_rtc.h"

/* linkit mt7697 HDK RTC 
 *
 * RTC start after init
 * After power-on board and init RTC, reboot board will not shutdown 
 * RTC except power off.
 *
 * Usage:
 *     from machine import RTC
 * 	   my_rtc = RTC()
 *
 * 	   # init 
 * 	   my_rtc.init() # enable RTC
 *
 * 	   # set new datetime: 
 * 	   new_datetime = (2019,1,28,14,22,30) tuple : (year, mon, day, hour, min, sec)
 * 	   my_rtc.datetime(new_datetime)   # The RTC is enabled 
 * 	                                   # if had not be enabled yet.
 *
 * 	   # get datetime 
 * 	   now = my_rtc.now() # return tuple of datetime 
 * 	                      # (year, mon, day, hour, min, sec)
 * 	                      # ex: (2019,1,28,14,22,35)
 *
 * 	   # Alarm : TODO
 * 	   # return 0 if alarm has expired
 * 	   my_rtc.alarm(alarm_id, time=time_ms, or datetime_tuple, *, repeat=False)
 *
 * */

static hal_rtc_status_t _rtc_set_datetime(uint16_t year, uint8_t mon, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec);

mp_obj_t machine_rtc_init(mp_obj_t self_in){
	if(HAL_RTC_STATUS_OK != hal_rtc_init()){
		nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "HAL RTC init error!!"));
	}
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_rtc_init_obj, machine_rtc_init);

mp_obj_t machine_rtc_deinit(mp_obj_t self_in){
	if(HAL_RTC_STATUS_OK != hal_rtc_deinit()){
		nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "HAL RTC deinit error!!"));
	}
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_rtc_deinit_obj, machine_rtc_deinit);

mp_obj_t machine_rtc_info(mp_obj_t self_in){
	// TODO
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_rtc_info_obj, machine_rtc_info);

mp_obj_t machine_rtc_datetime(mp_obj_t self_in, mp_obj_t datetime_tuple){
	mp_obj_t *items;
	mp_obj_get_array_fixed_n(datetime_tuple, 6, &items);
	hal_rtc_status_t res = _rtc_set_datetime(
		(uint16_t) mp_obj_get_int(items[0]),
		(uint8_t)mp_obj_get_int(items[1]),
		(uint8_t)mp_obj_get_int(items[2]),
		(uint8_t)mp_obj_get_int(items[3]),
		(uint8_t)mp_obj_get_int(items[4]),
		(uint8_t)mp_obj_get_int(items[5]));
	if(HAL_RTC_STATUS_OK != res){
		nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "HAL RTC set time failed!!"));
	}
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(machine_rtc_datetime_obj, machine_rtc_datetime);

mp_obj_t machine_rtc_now(mp_obj_t self_in){
	hal_rtc_time_t now;
	if(HAL_RTC_STATUS_OK != hal_rtc_get_time(&now)){
		nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "HAL RTC gets time error!!"));
	}
	mp_obj_t tuple[6] = {
		mp_obj_new_int( 2000 + ((mp_int_t)now.rtc_year)),
		mp_obj_new_int((mp_int_t) now.rtc_mon), 
		mp_obj_new_int((mp_int_t) now.rtc_day), 
		mp_obj_new_int((mp_int_t) now.rtc_hour), 
		mp_obj_new_int((mp_int_t) now.rtc_min), 
		mp_obj_new_int((mp_int_t) now.rtc_sec)
	};
	return mp_obj_new_tuple(6, tuple);
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_rtc_now_obj, machine_rtc_now);

typedef struct _machine_rtc_obj_t {
	mp_obj_base_t base;
}machine_rtc_obj_t;

STATIC const machine_rtc_obj_t machine_rtc_obj = {{&machine_rtc_type}};

STATIC mp_obj_t machine_rtc_make_new(const mp_obj_type_t * type, size_t n_args, size_t n_kw, const mp_obj_t * args){
	return MP_OBJ_FROM_PTR(&machine_rtc_obj);
}

static hal_rtc_status_t _rtc_set_datetime(
		uint16_t year,
		uint8_t mon,
		uint8_t day,
		uint8_t hour,
		uint8_t min,
		uint8_t sec
		){
	hal_rtc_time_t set_time = {0};
	set_time.rtc_year = year - _RTC_BASE_YEAR;
	set_time.rtc_mon  = mon;
	set_time.rtc_day  = day;
	set_time.rtc_hour = hour;
	set_time.rtc_min  = min;
	set_time.rtc_sec  = sec;
	return hal_rtc_set_time(&set_time);
}


STATIC const mp_rom_map_elem_t machine_rtc_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_rtc_init_obj) },
	{ MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_rtc_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&machine_rtc_info_obj) },
	{ MP_ROM_QSTR(MP_QSTR_datetime), MP_ROM_PTR(&machine_rtc_datetime_obj) },
    { MP_ROM_QSTR(MP_QSTR_now), MP_ROM_PTR(&machine_rtc_now_obj) },
    // { MP_ROM_QSTR(MP_QSTR_alarm), MP_ROM_PTR(&machine_rtc_alarm_obj) }, // TODO
    // { MP_ROM_QSTR(MP_QSTR_alarm_left), MP_ROM_PTR(&machine_rtc_alarm_left_obj) }, // TODO
    // { MP_ROM_QSTR(MP_QSTR_alarm_cancel), MP_ROM_PTR(&machine_rtc_alarm_cancel_obj) }, // TODO
    // { MP_ROM_QSTR(MP_QSTR_irq), MP_ROM_PTR(&machine_rtc_irq_obj) }, // TODO
    // { MP_ROM_QSTR(MP_QSTR_wakeup), MP_ROM_PTR(&machine_rtc_wakeup_obj) }, // TODO
    // { MP_ROM_QSTR(MP_QSTR_calibration), MP_ROM_PTR(&pyb_rtc_calibration_obj) }, // TODO
};
STATIC MP_DEFINE_CONST_DICT(machine_rtc_locals_dict, machine_rtc_locals_dict_table);


const mp_obj_type_t machine_rtc_type = {
	{ &mp_type_type },
	.name = MP_QSTR_RTC,
	.make_new = machine_rtc_make_new,
	.locals_dict = (mp_obj_dict_t*)&machine_rtc_locals_dict,
};
