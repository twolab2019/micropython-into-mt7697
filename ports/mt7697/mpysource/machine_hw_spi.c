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



#define SPIM_PIN_NUMBER_CS      HAL_GPIO_32
#define SPIM_PIN_FUNC_CS        HAL_GPIO_32_GPIO32
#define SPIM_PIN_NUMBER_CLK     HAL_GPIO_31
#define SPIM_PIN_FUNC_CLK       HAL_GPIO_31_SPI_SCK_M_CM4
#define SPIM_PIN_NUMBER_MOSI    HAL_GPIO_29
#define SPIM_PIN_FUNC_MOSI      HAL_GPIO_29_SPI_MOSI_M_CM4
#define SPIM_PIN_NUMBER_MISO    HAL_GPIO_30
#define SPIM_PIN_FUNC_MISO      HAL_GPIO_30_SPI_MISO_M_CM4

hw_spi_trans_slave_data  slave_user_data;

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
    bool                    is_master,
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
    if (is_master != self->is_master){
        self->is_master = is_master;
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

    if (sck != -2 && sck != self->sck) {
        self->sck = sck;
        changed = true;
    }

    if (mosi != -2 && mosi != self->mosi) {
        self->mosi = mosi;
        changed = true;
    }

    if (miso != -2 && miso != self->miso) {
        self->miso = miso;
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
    if(is_master) {
        hal_spi_master_config_t spi_config;
        /*Step2: initialize SPI master. */
        spi_config.bit_order = firstbit;
        spi_config.slave_port = HAL_SPI_MASTER_SLAVE_0;
        spi_config.clock_frequency = baudrate;
        spi_config.phase = phase;
        spi_config.polarity = polarity;
        if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_init(HAL_SPI_MASTER_0, &spi_config)) {
            printf("hal_spi_master_init fail\r\n");
            return;
        } else {
            printf("hal_spi_master_init pass\r\n");
        }

    } else {
        hal_spi_slave_config_t  spis_configure;
        spis_configure.phase = phase;
        spis_configure.polarity = polarity;
        /* Step2: Init SPI slaver. */
        if (HAL_SPI_SLAVE_STATUS_OK != hal_spi_slave_init(HAL_SPI_MASTER_0, &spis_configure)) {
            printf("hal_spi_slave_init fail\r\n");
            return;
        } else {
            printf("hal_spi_slave_init pass\r\n");
        }
        
        transfer_data_finished = false;
        /* Step3: Register callback to SPI slaver. */
        if (HAL_SPI_SLAVE_STATUS_OK != hal_spi_slave_register_callback(HAL_SPI_SLAVE_0, spis_user_callback, &slave_user_data)) {
            printf("hal_spi_slave_register_callback fail\r\n");
            return;
        } else {
            printf("hal_spi_slave_register_callback pass\r\n");
        }

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

STATIC void machine_hw_spi_set_slave_address(mp_obj_base_t *self_in, uint32_t address) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->address = address;
    spim_write_slave_address(address);
    spim_write_slave_data(SPI_TEST_WRITE_DATA_BEGIN);
    spim_write_slave_command(false);
    spim_wait_slave_idle();
    spim_active_slave_irq();
    spim_wait_slave_ack();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_hw_spi_set_slave_address_obj,  machine_hw_spi_set_slave_address);

STATIC void machine_hw_spi_send_data_to_slaver(mp_obj_base_t *self_in, size_t len, const uint8_t *src){
    uint8_t *src_data;
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    spim_write_slave_address(self->address + SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE);

    src_data = src;
    for (int i = 0; i < len / 4; i++) {
        spim_write_slave_data(src_data[i*4]);
        spim_write_slave_command(false);
        spim_wait_slave_idle();
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_hw_spi_send_data_to_slaver_obj, machine_hw_spi_send_data_to_slaver);

STATIC void machine_hw_spi_send_completely_wait_slave_ack(mp_obj_base_t *self_in) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    /* Step4: notice slave send completely and wait slave to ack. */
    spim_write_slave_address(self->address);
    spim_write_slave_data(SPI_TEST_WRITE_DATA_END);
    spim_write_slave_command(false);
    spim_active_slave_irq();
    spim_wait_slave_ack();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_hw_spi_send_completely_wait_slave_ack_obj, machine_hw_spi_send_completely_wait_slave_ack);

STATIC void machine_hw_spi_read_data_to_slaver(mp_obj_base_t *self_in, size_t len, const uint8_t *data){
    uint8_t *dest_data;
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    spim_write_slave_address(self->address + SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE);

    dest_data = data;
    for (int i = 0; i < len / 4; i++) {
        spim_write_slave_command(true);
        spim_wait_slave_idle();
        spim_wait_slave_idle();
        dest_data[i*4] = spim_read_slave_data();
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_hw_spi_read_data_to_slaver_obj, machine_hw_spi_read_data_to_slaver);

STATIC void machine_hw_spi_transfer(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest) {
    uint32_t time1;
    uint32_t time2;
    time1 = get_current_millisecond();

    slave_user_data.src = src;
    slave_user_data.dest = dest;
    slave_user_data.len = &len;

    /* Step4: waiting SPI master to transfer data with us. */
    memset(slaver_data_buffer, 0, SPI_TEST_DATA_SIZE + SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE);
    transfer_data_finished = false;
    while (transfer_data_finished == false){
        time2 = get_current_millisecond();
        if ((time2 - time1) >= 5000){
            break;
        }
    }
}

/******************************************************************************/
// MicroPython bindings for hw_spi

STATIC void machine_hw_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "SPI(is_mastter=%b, baudrate=%u, polarity=%u, phase=%u, bits=%u, firstbit=%u, sck=%d, mosi=%d, miso=%d)",
              self->is_master, self->baudrate, self->polarity,
              self->phase, self->bits, self->firstbit,
              self->sck, self->mosi, self->miso);
}

STATIC void machine_hw_spi_init(mp_obj_base_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *) self_in;

    enum { ARG_master, ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit, ARG_sck, ARG_mosi, ARG_miso };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_master,   MP_ARG_REQUIRED | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_sck,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_mosi,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_miso,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args),
                     allowed_args, args);
    int8_t sck, mosi, miso;

    if (args[ARG_sck].u_obj == MP_OBJ_NULL) {
        sck = -2;
    } else if (args[ARG_sck].u_obj == mp_const_none) {
        sck = -1;
    } else {
        sck = SPIM_PIN_NUMBER_CLK;
    }

    if (args[ARG_miso].u_obj == MP_OBJ_NULL) {
        miso = -2;
    } else if (args[ARG_miso].u_obj == mp_const_none) {
        miso = -1;
    } else {
        miso = SPIM_PIN_NUMBER_MISO;
    }

    if (args[ARG_mosi].u_obj == MP_OBJ_NULL) {
        mosi = -2;
    } else if (args[ARG_mosi].u_obj == mp_const_none) {
        mosi = -1;
    } else {
        mosi = SPIM_PIN_NUMBER_MOSI;
    }

    machine_hw_spi_init_internal(self, args[ARG_master].u_bool, args[ARG_baudrate].u_int,
                                 args[ARG_polarity].u_int, args[ARG_phase].u_int, args[ARG_bits].u_int,
                                 args[ARG_firstbit].u_int, sck, mosi, miso);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_hw_spi_init_obj, 1, machine_hw_spi_init);

mp_obj_t machine_hw_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_master, ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit, ARG_sck, ARG_mosi, ARG_miso };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_master,       MP_ARG_REQUIRED | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 500000} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_SPI_MASTER_CLOCK_POLARITY0} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_SPI_MASTER_CLOCK_PHASE0} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MICROPY_PY_MACHINE_SPI_MSB} },
        { MP_QSTR_sck,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_int = SPIM_PIN_NUMBER_CLK} },
        { MP_QSTR_mosi,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_int = SPIM_PIN_NUMBER_MOSI} },
        { MP_QSTR_miso,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_int = SPIM_PIN_NUMBER_MISO} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    machine_hw_spi_obj_t *self = m_new_obj(machine_hw_spi_obj_t);
    self->base.type = &machine_hw_spi_type;

    machine_hw_spi_init_internal(
        self,
        args[ARG_master].u_bool,
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

    { MP_ROM_QSTR(MP_QSTR_set_slave_address), MP_ROM_PTR(&machine_hw_spi_set_slave_address_obj) },
    { MP_ROM_QSTR(MP_QSTR_send_data_to_slaver), MP_ROM_PTR(&machine_hw_spi_send_data_to_slaver_obj) },
    { MP_ROM_QSTR(MP_QSTR_send_completely_wait_slave_ack), MP_ROM_PTR(&machine_hw_spi_send_completely_wait_slave_ack_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_data_to_slaver), MP_ROM_PTR(&machine_hw_spi_read_data_to_slaver) },
};
STATIC MP_DEFINE_CONST_DICT(mp_machine_hw_spi_locals_dict, machine_hw_spi_locals_dict_table);

STATIC const mp_machine_spi_p_t machine_hw_spi_p = {
    .init = machine_hw_spi_init,
    .deinit = machine_hw_spi_deinit,
    .transfer = machine_hw_spi_transfer,
};

const mp_obj_type_t machine_hw_spi_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPI,
    .print = machine_hw_spi_print,
    .make_new = machine_hw_spi_make_new,
    .protocol = &machine_hw_spi_p,
    .locals_dict = (mp_obj_dict_t *) &mp_machine_hw_spi_locals_dict,
};
