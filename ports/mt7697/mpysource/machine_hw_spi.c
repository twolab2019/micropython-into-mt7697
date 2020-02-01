/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 "Tom Lin" <tomlin.ntust@gmail.com>
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
#include <stdint.h>
#include <string.h>

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"
#include "mpconfigboard_common.h"
#include "mphalport.h"
#include "spi.h"
#include "extmod/machine_spi.h"
/* hal includes */
#include "hal.h"
#include "system_mt7687.h"
#include "top.h"

STATIC void machine_hw_spi_deinit_internal(machine_hw_spi_obj_t *self) {

    if (self->mode == MACHINE_HW_SPI_MASTER && HAL_SPI_MASTER_STATUS_OK != hal_spi_master_deinit(SPI_PORT_0)) {
        mp_raise_msg(&mp_type_OSError," SPI master deinit fail\r\n");
        return;
    }

    hal_gpio_deinit(SPI_PIN_NUMBER_CS);
    hal_gpio_deinit(SPI_PIN_NUMBER_CLK);
    hal_gpio_deinit(SPI_PIN_NUMBER_MOSI);
    hal_gpio_deinit(SPI_PIN_NUMBER_MISO);
}

STATIC void machine_hw_spi_init_internal(machine_hw_spi_obj_t  *self, mp_arg_val_t *args) {
    enum { ARG_mode, ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit, ARG_timeout};
    // if we're not initialized, then we're
    // implicitly 'changed', since this is the init routine
    bool changed = self->state != MACHINE_HW_SPI_STATE_INIT;


    machine_hw_spi_obj_t old_self = *self;
    if (args[ARG_mode].u_int != self->mode) {
        self->mode = args[ARG_mode].u_int;
        changed = true;
    }
    if (args[ARG_baudrate].u_int != self->baudrate) {
        if (args[ARG_baudrate].u_int < HAL_SPI_MASTER_CLOCK_MIN_FREQUENCY ||
                args[ARG_baudrate].u_int > HAL_SPI_MASTER_CLOCK_MAX_FREQUENCY) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Please, Set baudrate value between %d and %d \n"
                    ,HAL_SPI_MASTER_CLOCK_MIN_FREQUENCY , HAL_SPI_MASTER_CLOCK_MAX_FREQUENCY));
        }
        self->baudrate = args[ARG_baudrate].u_int;
        changed = true;
    }

    if (args[ARG_polarity].u_int != self->polarity) {
        self->polarity = args[ARG_polarity].u_int;
        changed = true;
    }

    if (args[ARG_phase].u_int != self->phase) {
        self->phase = args[ARG_phase].u_int;
        changed = true;
    }

    if (args[ARG_bits].u_int != self->bits) {
        self->bits = args[ARG_bits].u_int;
        changed = true;
    }

    if (args[ARG_firstbit].u_int != self->firstbit) {
        self->firstbit = args[ARG_firstbit].u_int;
        changed = true;
    }

    if (args[ARG_timeout].u_int != self->timeout) {
        self->timeout = args[ARG_timeout].u_int;
        changed = true;
    }

    if (changed) {
        if (self->state == MACHINE_HW_SPI_STATE_INIT) {
            self->state = MACHINE_HW_SPI_STATE_DEINIT;
            machine_hw_spi_deinit_internal(&old_self);
        }
    } else if (self->state == MACHINE_HW_SPI_STATE_INIT) {
         mp_raise_msg(&mp_type_OSError,"SPI inited");
    }

    /*Step1: init GPIO, set SPIM pinmux(if EPT tool hasn't been used to configure the related pinmux).*/
    hal_gpio_init(SPI_PIN_NUMBER_CS);
    hal_gpio_init(SPI_PIN_NUMBER_CLK);
    hal_gpio_init(SPI_PIN_NUMBER_MOSI);
    hal_gpio_init(SPI_PIN_NUMBER_MISO);
    if (self->mode == MACHINE_HW_SPI_MASTER) {
        hal_pinmux_set_function(SPI_PIN_NUMBER_CS,   SPIM_PIN_FUNC_CS);
        hal_pinmux_set_function(SPI_PIN_NUMBER_CLK,  SPIM_PIN_FUNC_CLK);
        hal_pinmux_set_function(SPI_PIN_NUMBER_MOSI, SPIM_PIN_FUNC_MOSI);
        hal_pinmux_set_function(SPI_PIN_NUMBER_MISO, SPIM_PIN_FUNC_MISO);

        hal_spi_master_config_t spi_config;
        // FIXME: Does the DMA matter? There are two
        /*Step2: initialize SPI master. */
        spi_config.bit_order = self->firstbit;
        spi_config.slave_port = SPI_PORT_0;
        spi_config.clock_frequency = self->baudrate;
        spi_config.phase = self->phase;
        spi_config.polarity = self->polarity;
        if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_init(SPI_PORT_0, &spi_config)) {
            mp_raise_msg(&mp_type_OSError,"SPI master init fail\r\n");
        }
    } else {
        hal_pinmux_set_function(SPI_PIN_NUMBER_CS,   SPIS_PIN_FUNC_CS);
        hal_pinmux_set_function(SPI_PIN_NUMBER_CLK,  SPIS_PIN_FUNC_CLK);
        hal_pinmux_set_function(SPI_PIN_NUMBER_MOSI, SPIS_PIN_FUNC_MOSI);
        hal_pinmux_set_function(SPI_PIN_NUMBER_MISO, SPIS_PIN_FUNC_MISO);

        hal_spi_slave_config_t  spi_config;
        spi_config.phase = self->phase;
        spi_config.polarity = self->polarity;
        if (HAL_SPI_MASTER_STATUS_OK != hal_spi_slave_init(SPI_PORT_0, &spi_config)) {
            mp_raise_msg(&mp_type_OSError,"SPI slave init fail\r\n");
        }
    }
    self->state = MACHINE_HW_SPI_STATE_INIT;
}

STATIC mp_obj_t machine_hw_spi_deinit(mp_obj_base_t *self_in) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *) self_in;
    if (self->state == MACHINE_HW_SPI_STATE_INIT) {
        self->state = MACHINE_HW_SPI_STATE_DEINIT;
        machine_hw_spi_deinit_internal(self);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_hw_spi_deinit_obj, machine_hw_spi_deinit);

STATIC void machine_hw_spi_transfer(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t*)self_in;
    uint32_t time1;
    uint32_t time2;
    time1 = get_current_millisecond();
    hal_spi_master_send_and_receive_config_t spi_send_and_receive_config;

    /* Step4: waiting SPI master to transfer data with us. */
    transfer_data_finished = false;

    if (self->mode == MACHINE_HW_SPI_SLAVE) {
        hw_spi_trans_slave_data  slave_user_data;
        slave_user_data.src = src;
        slave_user_data.dest = dest;
        slave_user_data.len = &len;
        if (HAL_SPI_SLAVE_STATUS_OK != hal_spi_slave_register_callback(SPI_PORT_0, spis_user_callback, &slave_user_data))
            mp_raise_msg(&mp_type_OSError,"SPI Slave regiseter callback fail\r\n");
    }

    while (transfer_data_finished == false){
        time2 = get_current_millisecond();
        if (self->mode == MACHINE_HW_SPI_MASTER) {
            if (dest == NULL){
                if (HAL_SPI_MASTER_STATUS_OK == hal_spi_master_send_polling(SPI_PORT_0, src, len)) {
                    transfer_data_finished = true;
                }
            } else {
                spi_send_and_receive_config.receive_length = len;
                spi_send_and_receive_config.receive_buffer = dest;
                if (src == dest) {
                    spi_send_and_receive_config.send_length = 0;
                    spi_send_and_receive_config.send_data = NULL;
                } else {
                    spi_send_and_receive_config.send_length = len;
                    spi_send_and_receive_config.send_data = src;
                }
                if (HAL_SPI_MASTER_STATUS_OK == hal_spi_master_send_and_receive_polling(HAL_SPI_MASTER_0,&spi_send_and_receive_config))             {
                    transfer_data_finished = true;
                }
            }
        }
        if ((time2 - time1) >= self->timeout){
            mp_raise_msg(&mp_type_OSError,"SPI Transfer time out\r\n");
        }
    }
}

/******************************************************************************/
// MicroPython bindings for hw_spi

STATIC void machine_hw_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "SPI(mode=%s, baudrate=%u, polarity=%u, phase=%u, bits=%u, firstbit=%u, cs=p10, mosi=p11 miso=p12, sck=p13)",
              self->mode == MACHINE_HW_SPI_MASTER?"MASTER":"SLAVE", self->baudrate, self->polarity,
              self->phase, self->bits, self->firstbit);
}

STATIC mp_obj_t machine_hw_spi_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,     MP_ARG_REQUIRED|MP_ARG_INT , {.u_int = MACHINE_HW_SPI_MASTER} },
        { MP_QSTR_baudrate, MP_ARG_INT                 , {.u_int = 1000000} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_SPI_MASTER_CLOCK_POLARITY0} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_SPI_MASTER_CLOCK_PHASE0} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_SPI_MASTER_MSB_FIRST} },
        { MP_QSTR_timeout,  MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    };

    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args),
                     allowed_args, args);

    machine_hw_spi_init_internal(self, args);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_hw_spi_init_obj, 1, machine_hw_spi_init);

mp_obj_t machine_hw_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,     MP_ARG_INT                 , {.u_int = MACHINE_HW_SPI_MASTER} },
        { MP_QSTR_baudrate, MP_ARG_INT                 , {.u_int = 1000000} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_SPI_MASTER_CLOCK_POLARITY0} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_SPI_MASTER_CLOCK_PHASE0} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_SPI_MASTER_MSB_FIRST} },
        { MP_QSTR_timeout,  MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    };
    if (all_args[0] !=  MP_OBJ_NEW_SMALL_INT(0)) {
        mp_raise_ValueError("There is only one set SPI peripheral in MT7697.Id is 0");
    }
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args - 1, n_kw , all_args + 1, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    machine_hw_spi_obj_t *self = m_new_obj(machine_hw_spi_obj_t);
    self->base.type = &machine_hw_spi_type;

    machine_hw_spi_init_internal(self, args);

    return MP_OBJ_FROM_PTR(self);
}
STATIC const mp_rom_map_elem_t machine_hw_spi_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_hw_spi_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_hw_spi_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_machine_spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_machine_spi_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_machine_spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_readinto), MP_ROM_PTR(&mp_machine_spi_write_readinto_obj) },

    { MP_ROM_QSTR(MP_QSTR_MASTER), MP_ROM_INT(MACHINE_HW_SPI_MASTER) },
    { MP_ROM_QSTR(MP_QSTR_SLAVE), MP_ROM_INT(MACHINE_HW_SPI_SLAVE) },
    { MP_ROM_QSTR(MP_QSTR_MSB), MP_ROM_INT(MICROPY_PY_MACHINE_SPI_MSB) },
    { MP_ROM_QSTR(MP_QSTR_LSB), MP_ROM_INT(MICROPY_PY_MACHINE_SPI_LSB) },
};

STATIC MP_DEFINE_CONST_DICT(mp_machine_hw_spi_locals_dict, machine_hw_spi_locals_dict_table);

STATIC const mp_machine_spi_p_t machine_hw_spi_p = {
    .init = machine_hw_spi_init,
    .deinit = machine_hw_spi_deinit,
    .transfer = machine_hw_spi_transfer,};

const mp_obj_type_t machine_hw_spi_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPI,
    .print = machine_hw_spi_print,
    .make_new = machine_hw_spi_make_new,
    .protocol = &machine_hw_spi_p,
    .locals_dict = (mp_obj_dict_t *) &mp_machine_hw_spi_locals_dict,
};
