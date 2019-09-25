/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2018 Damien P. George
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
#include <stdarg.h>

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "lib/utils/interrupt_char.h"
#include "uart.h"

/*
 * UART(1,baudrate=9600,bits=8,parity=None,stop=1)
 *
 * only support UART 1
 *
 * baudrate :
 *     110, 300, 1200, 2400, 4800, 9600(defualt), 19200, 38400, 57600,
 *     115200, 230400, 460800, 921600
 *
 * bits :
 *     5, 6, 7, 8 (defualt)
 *
 * parity :
 *     None (or 0)
 *     1 (ODD)
 *     2 (EVEN)
 *
 * stop :
 *     1, 2
 *
 * Usage:
 *     from machine import UART
 *     uart = UART(1,115200)
 *     uart = UART(1,115200,8,None,1)
 *
 * */

STATIC void machine_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->is_enabled) {
        mp_printf(print, "UART(%u)", self->uart_id);
    } else {
        mp_printf(print, "UART(%u, baudrate=%u, bits=%u, parity=",
            self->uart_id, self->baudrate, self->char_width);
        if (self->parity == HAL_UART_PARITY_NONE) {
            mp_print_str(print, "None");
        } else if (!self->parity == HAL_UART_PARITY_ODD) {
            mp_print_str(print, "ODD");
        } else {
            mp_print_str(print, "EVEN");
        }

        mp_printf(print, ", stop=%u, flow=",
            ((self->stop) == 0 ? 1 : 2));
        if (!self->is_flow)
            mp_print_str(print, "false");
        else
            mp_print_str(print, "CTS|RTS");

        mp_printf(print, ", timeout=%u, rxbuf= 128 txbuf = 128)",
            self->timeout); 
    }
}

STATIC mp_obj_t machine_uart_init_helper(machine_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_uart_id, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 9600} },
        { MP_QSTR_bits, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_parity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = HAL_UART_PARITY_NONE} },
        { MP_QSTR_stop, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_flow, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
//        { MP_QSTR_timeout_char, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_rxbuf, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_read_buf_len, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 10} }, // legacy
    };

    // parse args
    struct {
        mp_arg_val_t uart_id, baudrate, bits, parity, stop, flow, timeout, rxbuf, read_buf_len;
    } args;
    mp_arg_parse_all(n_args, pos_args, kw_args,
        MP_ARRAY_SIZE(allowed_args), allowed_args, (mp_arg_val_t*)&args);


    // static UARTs are used for internal purposes and shouldn't be reconfigured
    if (self->is_static) {
        mp_raise_ValueError("UART is static and can't be init'd");
    }

    // uart id
    switch (args.uart_id.u_int) {
        case PYB_UART_0:
            self->uart_id = PYB_UART_0;
            break;
        case PYB_UART_1:
            self->uart_id = PYB_UART_1;
            break;
        default:
            // UART does not exist or is not configured for this board
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) does not exist", args.uart_id.u_int));
            return mp_const_false;
    }

    // baudrate
    uint32_t baudrate = args.baudrate.u_int;

    // parity
    uint32_t parity;
    switch(args.parity.u_int) {
        case HAL_UART_PARITY_NONE:
            parity = HAL_UART_PARITY_NONE;
            break;
        case HAL_UART_PARITY_ODD:
            parity = HAL_UART_PARITY_ODD;
            break;
        case HAL_UART_PARITY_EVEN:
            parity = HAL_UART_PARITY_EVEN;
            break;
        default:
            mp_raise_ValueError("unsupported combination of parity");
            return mp_const_false;

    }

    // number of bits
    uint32_t bits = args.bits.u_int;
    if (bits < 5 || bits > 8) {
        mp_raise_ValueError("unsupported combination of bits");
    }

    // stop bits
    uint32_t stop;
    switch (args.stop.u_int) {
        case 1: stop = HAL_UART_STOP_BIT_1; break;
        default: stop = HAL_UART_STOP_BIT_2; break;
    }

    // flow control
    uint32_t flow = args.flow.u_bool;

    // init UART (if it fails, it's because the port doesn't exist)
    if (!uart_init(self, baudrate, bits, parity, stop, flow)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) doesn't exist", self->uart_id));
    }

    // set timeout
    self->timeout = args.timeout.u_int;

    // setup the read buffer
    if (args.rxbuf.u_int >= 0) {
        // rxbuf overrides legacy read_buf_len
        args.read_buf_len.u_int = args.rxbuf.u_int;
    }
    if (args.read_buf_len.u_int <= 0) {
        // no read buffer
        uart_set_rxbuf(self, 0, NULL);
    } else {
        // read buffer using interrupts
        size_t len = args.read_buf_len.u_int + 1; // +1 to adjust for usable length of buffer
        uint8_t *buf = m_new(byte, len << self->char_width);
        uart_set_rxbuf(self, len, buf);
    }

    return mp_const_none;
}

/// \classmethod \constructor(bus, ...)
///
/// Construct a UART object on the given bus.  `bus` can be P0, P1, P6, P7
/// With no additional parameters, the UART object is created but not
/// initialised (it has the settings from the last initialisation of
/// the bus, if any).  If extra arguments are given, the bus is initialised.
/// See `init` for parameters of initialisation.
///
/// The physical pins of the UART busses are:
///
///   - `UART(0)` is on : `(RX, TX) = (GPIO2, GPIO3) = (P0, P1)`
///   - `UART(1)` is on : `(RX, TX) = (GPIO36, GPIO37) = (P7, P6)`
STATIC mp_obj_t machine_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // work out port
    mp_int_t uart_id = mp_obj_get_int(args[0]);
    if (uart_id < PYB_UART_0 || uart_id >= PYB_UART_MAX) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) does not exist", uart_id));
    }

    // Attempts to use UART0 from Python has resulted in all sorts of fun errors.
    // FIXME: UART0 is disabled for now.
    if (uart_id == PYB_UART_0) {
       nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) is disabled (dedicated to REPL)", uart_id));
    }

    machine_uart_obj_t *self;
    if (MP_STATE_PORT(machine_uart_obj_all)[uart_id] == NULL) {
        // create new UART object
        self = m_new0(machine_uart_obj_t, 1);
        self->base.type = &machine_uart_type;
        self->uart_id = uart_id;
        MP_STATE_PORT(machine_uart_obj_all)[uart_id] = self;
    } else {
        // reference existing UART object
        self = MP_STATE_PORT(machine_uart_obj_all)[uart_id];
    }

    if (n_args > 1 || n_kw > 0) {
        // start the peripheral
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        printf("uart_id = %d\n", self->uart_id);
        //printf("0 rate = %d\n", mp_obj_get_int(args[1]));
        machine_uart_init_helper(self, n_args , args, &kw_args);
    }

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_uart_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    printf("n_args = %d \n", n_args);
    return machine_uart_init_helper(MP_OBJ_TO_PTR(args[0]), n_args -1 , args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_uart_init_obj, 1, machine_uart_init);

/// \method deinit()
/// Turn off the UART bus.
STATIC mp_obj_t machine_uart_deinit(mp_obj_t self_in) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->uart_id == PYB_UART_0) {
       nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) is disabled (dedicated to REPL)", self->uart_id));
    }
    uart_deinit(self);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_uart_deinit_obj, machine_uart_deinit);

/// \method any()
/// Return `True` if any characters waiting, else `False`.
STATIC mp_obj_t machine_uart_any(mp_obj_t self_in) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_NEW_SMALL_INT(uart_rx_any(self));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_uart_any_obj, machine_uart_any);

/// \method writechar(char)
/// Write a single character on the bus.  `char` is an integer to write.
/// Return value: `None`.
STATIC mp_obj_t machine_uart_writechar(mp_obj_t self_in, mp_obj_t char_in) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // get the character to write (might be 9 bits)
    uint16_t data = mp_obj_get_int(char_in);

    // write the character
    int errcode;
    if (uart_tx_data(self, &data, 1, &errcode) == 0) {
        errcode = MP_ETIMEDOUT;
    }

    if (errcode != 0) {
        mp_raise_OSError(errcode);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_uart_writechar_obj, machine_uart_writechar);

/// \method readchar()
/// Receive a single character on the bus.
/// Return value: The character read, as an integer.  Returns -1 on timeout.
STATIC mp_obj_t machine_uart_readchar(mp_obj_t self_in) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t readcahr = uart_rx_char(self);
    if (readcahr != -1) {
        return MP_OBJ_NEW_SMALL_INT(readcahr);
    } else {
        // return -1 on timeout
        return MP_OBJ_NEW_SMALL_INT(-1);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_uart_readchar_obj, machine_uart_readchar);


STATIC const mp_rom_map_elem_t machine_uart_locals_dict_table[] = {
    // instance methods

    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_uart_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_uart_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_any), MP_ROM_PTR(&machine_uart_any_obj) },

    /// \method read([nbytes])
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    /// \method readline()
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj)},
    /// \method readinto(buf[, nbytes])
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    /// \method write(buf)
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },

    { MP_ROM_QSTR(MP_QSTR_writechar), MP_ROM_PTR(&machine_uart_writechar_obj) },
    { MP_ROM_QSTR(MP_QSTR_readchar), MP_ROM_PTR(&machine_uart_readchar_obj) },

    { MP_ROM_QSTR(MP_QSTR_NONE), MP_ROM_INT(HAL_UART_PARITY_NONE)},
    { MP_ROM_QSTR(MP_QSTR_ODD), MP_ROM_INT(HAL_UART_PARITY_ODD)},
    { MP_ROM_QSTR(MP_QSTR_EVEN), MP_ROM_INT(HAL_UART_PARITY_EVEN)},

};

STATIC MP_DEFINE_CONST_DICT(machine_uart_locals_dict, machine_uart_locals_dict_table);

STATIC mp_uint_t machine_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    byte *buf = buf_in;

    // read the data
    byte *orig_buf = buf;
    for (;;) {
        uint32_t data = uart_rx_char(self);
        if (data == -1){
            return buf - orig_buf;
        }
        *buf++ = data;
        if (--size == 0) {
            // return number of bytes read
            return buf - orig_buf;
        } else if (self->event_transaction_error) {
            *errcode = MP_EAGAIN;
            return MP_STREAM_ERROR;

        }
    }
}

STATIC mp_uint_t machine_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const byte *buf = buf_in;

    // write the data
    size_t num_tx = uart_tx_data(self, buf, size , errcode);

    if (*errcode == 0 || *errcode == MP_ETIMEDOUT) {
        // return number of bytes written, even if there was a timeout
        return num_tx;
    } else {
        return MP_STREAM_ERROR;
    }
}

STATIC mp_uint_t machine_uart_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_uint_t ret;
    if (request == MP_STREAM_POLL) {
        uintptr_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && uart_rx_any(self)) {
            ret |= MP_STREAM_POLL_RD;
        }
        if ((flags & MP_STREAM_POLL_WR) && 1) {
            ret |= MP_STREAM_POLL_WR;
        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

STATIC const mp_stream_p_t uart_stream_p = {
    .read = machine_uart_read,
    .write = machine_uart_write,
    .ioctl = machine_uart_ioctl,
    .is_text = false,
};

const mp_obj_type_t machine_uart_type = {
    { &mp_type_type },
    .name = MP_QSTR_UART,
    .print = machine_uart_print,
    .make_new = machine_uart_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &uart_stream_p,
    .locals_dict = (mp_obj_dict_t*)&machine_uart_locals_dict,
};
