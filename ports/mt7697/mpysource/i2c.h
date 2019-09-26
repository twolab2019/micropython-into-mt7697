/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Tom Lin <tomlin.ntust@gmail.com>
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

#ifndef MICROPY_INCLUDED_MT7697_HW_I2C_H
#define MICROPY_INCLUDED_MT7697_HW_I2C_H
/* hal includes */
#include "hal.h"
#include "system_mt7687.h"
#include "top.h"
typedef struct _machine_hard_i2c_obj_t {
    mp_obj_base_t base;
    uint8_t  id;
    uint32_t freq;
    uint32_t timeout;
    enum {
        MACHINE_HW_I2C_STATE_NONE,
        MACHINE_HW_I2C_STATE_INIT,
        MACHINE_HW_I2C_STATE_DEINIT,
    } state; 
} machine_hard_i2c_obj_t;

STATIC const mp_arg_t machine_i2c_mem_allowed_args[] = {
    { MP_QSTR_addr,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_memaddr, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_arg,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_addrsize, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
};

MP_DECLARE_CONST_FUN_OBJ_KW(machine_i2c_init_obj);
//MP_DECLARE_CONST_FUN_OBJ_1(machine_i2c_scan_obj);
MP_DECLARE_CONST_FUN_OBJ_1(machine_i2c_start_obj);
MP_DECLARE_CONST_FUN_OBJ_1(machine_i2c_stop_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_readinto_obj);
MP_DECLARE_CONST_FUN_OBJ_2(machine_i2c_write_obj);

MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_readfrom_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_readfrom_into_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_writeto_obj);

MP_DECLARE_CONST_FUN_OBJ_KW(machine_i2c_readfrom_mem_obj);
MP_DECLARE_CONST_FUN_OBJ_KW(machine_i2c_readfrom_mem_into_obj);
MP_DECLARE_CONST_FUN_OBJ_KW(machine_i2c_writeto_mem_obj);

extern const mp_obj_type_t machine_hard_i2c_type; 
#endif 
