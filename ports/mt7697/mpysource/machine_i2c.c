/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Tom Lin (tomlin.ntust@gmail.com)
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
#include <string.h>

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "extmod/machine_i2c.h"
#include "i2c.h"

#if MICROPY_PY_MACHINE_I2C

STATIC void machine_hard_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hard_i2c_obj_t *self = self_in;
    mp_printf(print, "I2C(0, scl=P8, sda=P9, freq=%u, timeout=%u)", self->freq, self->timeout);
}

STATIC void machine_hard_i2c_init(machine_hard_i2c_obj_t *self, uint32_t freq, uint32_t timeout) {
    // set parameters
    hal_i2c_config_t i2c_init;
    int error_status;
    
    hal_i2c_frequency_t input_frequency;
    if (freq == 50000)
        input_frequency = 0;
    else if (freq == 1000000)
        input_frequency = 5;
    else {
        input_frequency = freq / 100000;
    }

    hal_i2c_port_t i2c_port = HAL_I2C_MASTER_0;

    /*Step1: Initialize GPIO, set GPIO pinmux(if EPT tool is not used to configure the related pinmux).*/
    hal_gpio_init(HAL_GPIO_27);
    hal_gpio_init(HAL_GPIO_28);
    hal_pinmux_set_function(HAL_GPIO_27, HAL_GPIO_27_I2C1_CLK);
    hal_pinmux_set_function(HAL_GPIO_28, HAL_GPIO_28_I2C1_DATA);
    hal_gpio_pull_up(HAL_GPIO_27);
    hal_gpio_pull_up(HAL_GPIO_28);
    
    i2c_init.frequency = input_frequency;
    error_status = hal_i2c_master_init(i2c_port, &i2c_init);
    if (HAL_I2C_STATUS_OK != error_status) {
        if (error_status == HAL_I2C_STATUS_INVALID_PARAMETER)
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "HAL I2C init error!! An invalid transfer_frequency is given"));
        else
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "HAL I2C init error!! The i2c bus is use"));
    } else {
        self->base.type = &machine_hard_i2c_type;
        self->id = HAL_I2C_MASTER_0;
        self->freq = freq; 
        self->timeout = timeout;
        self->state = MACHINE_HW_I2C_STATE_INIT;
    }
}

STATIC mp_obj_t machine_i2c_deinit(mp_obj_t self_in) {
    machine_hard_i2c_obj_t *self = self_in;
    uint32_t time1;
    uint32_t time2;
    time1 = get_current_millisecond();
    bool transfer_data_finished = false;
    if (self->state != MACHINE_HW_I2C_STATE_INIT)
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "HAL I2C deinit error!! I2C hasn't inited"));
    while (transfer_data_finished == false) {
        if (HAL_I2C_STATUS_OK == hal_i2c_master_deinit(HAL_I2C_MASTER_0)) {
            transfer_data_finished = true;
            self->state = MACHINE_HW_I2C_STATE_DEINIT;
            break;
        }
        time2 = get_current_millisecond();
        if ((time2 - time1) >= self->timeout){
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "I2C deinit error"));
            break;
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_i2c_deinit_obj, machine_i2c_deinit);

/******************************************************************************/
/* MicroPython bindings for machine API                                       */
mp_obj_t machine_hard_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_freq, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_freq, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 400000} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },
    };
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get static peripheral object
    int i2c_id = mp_obj_get_int(args[ARG_id].u_obj);;
    if (i2c_id != 0) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "I2C(%d) doesn't exist. I2C port is 0", i2c_id));
    } 
    
    machine_hard_i2c_obj_t *self = m_new_obj(machine_hard_i2c_obj_t);
    self->state = MACHINE_HW_I2C_STATE_NONE;
    // initialise the I2C peripheral
    machine_hard_i2c_init(self, args[ARG_freq].u_int, args[ARG_timeout].u_int);

    return MP_OBJ_FROM_PTR(self);
}

int machine_hard_i2c_readfrom(mp_obj_base_t *self_in, uint16_t addr, uint8_t *dest, size_t len, bool stop) {
    machine_hard_i2c_obj_t *self = (machine_hard_i2c_obj_t *)self_in;
    uint32_t time1;
    uint32_t time2;
    time1 = get_current_millisecond();
    bool transfer_data_finished = false;
    while (transfer_data_finished == false) {
        if (HAL_I2C_STATUS_OK == hal_i2c_master_receive_polling(HAL_I2C_MASTER_0, addr, dest, len)) {
            transfer_data_finished = true;
            break;
        }
        time2 = get_current_millisecond();
        if ((time2 - time1) >= self->timeout){
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "I2C readfrom error"));
            break;
        }
    }
    return 0;
}

int machine_hard_i2c_writeto(mp_obj_base_t *self_in, uint16_t addr, const uint8_t *src, size_t len, bool stop) {
    machine_hard_i2c_obj_t *self = (machine_hard_i2c_obj_t *)self_in;
    uint32_t time1;
    uint32_t time2;
    time1 = get_current_millisecond();
    bool transfer_data_finished = false;
    while (transfer_data_finished == false) {
        if (HAL_I2C_STATUS_OK == hal_i2c_master_send_polling(HAL_I2C_MASTER_0, addr, src, len)) {
            transfer_data_finished = true;
            break;
        }
        time2 = get_current_millisecond();
        if ((time2 - time1) >= self->timeout){
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "I2C writeto error"));
            break;
        }
    }
    return len;
}

STATIC mp_obj_t machine_i2c_scan(mp_obj_t self_in) {
    machine_hard_i2c_obj_t *self = (machine_hard_i2c_obj_t *)self_in;
    mp_obj_t list = mp_obj_new_list(0, NULL);
    uint8_t send_data = 0x0;

    int error_status = HAL_I2C_STATUS_OK;
    for (int addr = 0x08; addr < 0x78; ++addr) {
        error_status = hal_i2c_master_send_polling(HAL_I2C_MASTER_0, addr, &send_data, 0);
        if (HAL_I2C_STATUS_OK == error_status) {
            mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(addr));
        }
    }
    return list;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_hard_i2c_scan_obj, machine_i2c_scan);

STATIC const mp_rom_map_elem_t machine_hw_i2c_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_i2c_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&machine_hard_i2c_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_i2c_deinit_obj)},

    // standard bus operations
    { MP_ROM_QSTR(MP_QSTR_readfrom), MP_ROM_PTR(&machine_i2c_readfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto), MP_ROM_PTR(&machine_i2c_writeto_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_machine_hw_i2c_locals_dict, machine_hw_i2c_locals_dict_table);

STATIC const mp_machine_i2c_p_t machine_hard_i2c_p = {
    .readfrom = machine_hard_i2c_readfrom,
    .writeto = machine_hard_i2c_writeto,
};

const mp_obj_type_t machine_hard_i2c_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = machine_hard_i2c_print,
    .make_new = machine_hard_i2c_make_new,
    .protocol = &machine_hard_i2c_p,
    .locals_dict = (mp_obj_dict_t*)&mp_machine_hw_i2c_locals_dict,
};

#endif // MICROPY_PY_MACHINE_I2C
