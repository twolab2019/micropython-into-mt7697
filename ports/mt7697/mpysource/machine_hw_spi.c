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
	if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_deinit(HAL_SPI_MASTER_0)) {
        printf("  SPI master deinit fail\r\n");
        return;
    }

    hal_gpio_deinit(SPIM_PIN_NUMBER_CS);
    hal_gpio_deinit(SPIM_PIN_NUMBER_CLK);
    hal_gpio_deinit(SPIM_PIN_NUMBER_MOSI);
    hal_gpio_deinit(SPIM_PIN_NUMBER_MISO);
}

STATIC void machine_hw_spi_init_internal(
    machine_hw_spi_obj_t    *self,
    int8_t                  id,
    int32_t                 baudrate,
    int8_t                  polarity,
    int8_t                  phase,
    int8_t                  bits,
    int8_t                  firstbit,
    int8_t                  sck,
    int8_t                  mosi,
    int8_t                  miso) {

    // if we're not initialized, then we're
    // implicitly 'changed', since this is the init routine
    bool changed = self->state != MACHINE_HW_SPI_STATE_INIT;


    machine_hw_spi_obj_t old_self = *self;
    if (id != -1 && id != self->id) {
        self->id = id;
        changed = true;
    }

    if (baudrate != -1 && baudrate != self->baudrate) {
        self->baudrate = baudrate;
        changed = true;
    }

    if (polarity != -1 && polarity != self->polarity) {
        self->polarity = polarity;
        changed = true;
    }

    if (phase != -1 && phase != self->phase) {
        self->phase =  phase;
        changed = true;
    }

    if (bits != -1 &&  bits != self->bits) {
        self->bits = bits;
        changed = true;
    }

    if (firstbit != -1 && firstbit != self->firstbit) {
        self->firstbit = firstbit;
        changed = true;
    }

    if (changed) {
        if (self->state == MACHINE_HW_SPI_STATE_INIT) {
            self->state = MACHINE_HW_SPI_STATE_DEINIT;
            machine_hw_spi_deinit_internal(&old_self);
        }
    } else {
        return; // no changes
    }

    /*Step1: init GPIO, set SPIM pinmux(if EPT tool hasn't been used to configure the related pinmux).*/
    hal_gpio_init(SPIM_PIN_NUMBER_CS);
    hal_gpio_init(SPIM_PIN_NUMBER_CLK);
    hal_gpio_init(SPIM_PIN_NUMBER_MOSI);
    hal_gpio_init(SPIM_PIN_NUMBER_MISO);
    hal_pinmux_set_function(SPIM_PIN_NUMBER_CS, SPIM_PIN_FUNC_CS);
    hal_pinmux_set_function(SPIM_PIN_NUMBER_CLK, SPIM_PIN_FUNC_CLK);
    hal_pinmux_set_function(SPIM_PIN_NUMBER_MOSI, SPIM_PIN_FUNC_MOSI);
    hal_pinmux_set_function(SPIM_PIN_NUMBER_MISO, SPIM_PIN_FUNC_MISO);

    // FIXME: Does the DMA matter? There are two
    hal_spi_master_config_t spi_config;
    /*Step2: initialize SPI master. */
    spi_config.bit_order = firstbit;
    spi_config.slave_port = id;
    spi_config.clock_frequency = baudrate;
    spi_config.phase = phase;
    spi_config.polarity = polarity;
    if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_init(HAL_SPI_MASTER_0, &spi_config)) {
        printf("hal_spi_master_init fail\r\n");
        return;
    } else {
        printf("hal_spi_master_init pass\r\n");
    }

    self->state = MACHINE_HW_SPI_STATE_INIT;
}

STATIC void machine_hw_spi_deinit(mp_obj_base_t *self_in) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *) self_in;
    if (self->state == MACHINE_HW_SPI_STATE_INIT) {
        self->state = MACHINE_HW_SPI_STATE_DEINIT;
        machine_hw_spi_deinit_internal(self);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_hw_spi_deinit_obj, machine_hw_spi_deinit);

STATIC void machine_hw_spi_transfer(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t*)self_in;
    uint32_t time1;
    uint32_t time2;
    time1 = get_current_millisecond();
    hal_spi_master_send_and_receive_config_t spi_send_and_receive_config;

    /* Step4: waiting SPI master to transfer data with us. */
    bool transfer_data_finished = false;

    while (transfer_data_finished == false){
        time2 = get_current_millisecond();
        if (dest == NULL){
            printf("Transfer write\n");
            if (HAL_SPI_MASTER_STATUS_OK == hal_spi_master_send_polling(self->id, src, len)) {
                printf("Transfer write success\n");
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
            if (HAL_SPI_MASTER_STATUS_OK == hal_spi_master_send_and_receive_polling(HAL_SPI_MASTER_0,&spi_send_and_receive_config)) {
                printf("Transfer send and recevie success\n");
                transfer_data_finished = true;
            }
        }
        if ((time2 - time1) >= 5000){
            printf("Transfer time out\n");
            break;
        }
    }
}

/******************************************************************************/
// MicroPython bindings for hw_spi

STATIC void machine_hw_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "SPI(master_id=%u, baudrate=%u, polarity=%u, phase=%u, bits=%u, firstbit=%u, cs=p10, mosi=p11 miso=p12, sck=p13)",
              self->id, self->baudrate, self->polarity,
              self->phase, self->bits, self->firstbit
              );
}

STATIC void machine_hw_spi_init(mp_obj_base_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *) self_in;

    enum { ARG_id, ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit, ARG_sck, ARG_mosi, ARG_miso };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_INT                 , {.u_int = 0} },
        { MP_QSTR_baudrate, MP_ARG_INT                 , {.u_int = 1000000} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_SPI_MASTER_CLOCK_POLARITY0} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_SPI_MASTER_CLOCK_PHASE0} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_SPI_MASTER_MSB_FIRST} },
        { MP_QSTR_sck,      MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = SPIM_PIN_NUMBER_CLK} },
        { MP_QSTR_mosi,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = SPIM_PIN_NUMBER_MOSI} },
        { MP_QSTR_miso,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = SPIM_PIN_NUMBER_MISO} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args),
                     allowed_args, args);

    machine_hw_spi_init_internal(self, args[ARG_id].u_int, args[ARG_baudrate].u_int,
                                 args[ARG_polarity].u_int, args[ARG_phase].u_int, args[ARG_bits].u_int,
                                 args[ARG_firstbit].u_int, args[ARG_sck].u_int, args[ARG_mosi].u_int, args[ARG_miso].u_int);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_hw_spi_init_obj, 1, machine_hw_spi_init);

mp_obj_t machine_hw_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit, ARG_sck, ARG_mosi, ARG_miso };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_INT                 , {.u_int = HAL_SPI_MASTER_0} },
        { MP_QSTR_baudrate, MP_ARG_INT                 , {.u_int = 1000000} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_SPI_MASTER_CLOCK_POLARITY0} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_SPI_MASTER_CLOCK_PHASE0} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_SPI_MASTER_MSB_FIRST} },
        { MP_QSTR_sck,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_int = SPIM_PIN_NUMBER_CLK} },
        { MP_QSTR_mosi,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_int = SPIM_PIN_NUMBER_MOSI} },
        { MP_QSTR_miso,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_int = SPIM_PIN_NUMBER_MISO} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    machine_hw_spi_obj_t *self = m_new_obj(machine_hw_spi_obj_t);
    self->base.type = &machine_hw_spi_type;

    args[ARG_sck].u_int = SPIM_PIN_NUMBER_CLK;
    args[ARG_mosi].u_int = SPIM_PIN_NUMBER_MOSI;
    args[ARG_miso].u_int = SPIM_PIN_NUMBER_MISO;

    machine_hw_spi_init_internal(
        self,
        args[ARG_id].u_int,
        args[ARG_baudrate].u_int,
        args[ARG_polarity].u_int,
        args[ARG_phase].u_int,
        args[ARG_bits].u_int,
        args[ARG_firstbit].u_int,
        args[ARG_sck].u_int,
        args[ARG_mosi].u_int,
        args[ARG_miso].u_int);

    return MP_OBJ_FROM_PTR(self);
}
STATIC const mp_rom_map_elem_t machine_hw_spi_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_hw_spi_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_hw_spi_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_machine_spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_machine_spi_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_machine_spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_readinto), MP_ROM_PTR(&mp_machine_spi_write_readinto_obj) },

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
