/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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
#include "py/ringbuf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "hal.h"
#include "sys_init.h"
#include "mpconfigboard_common.h"
#include "mphalport.h"

extern void NORETURN __fatal_error(const char *msg);

/*
 * config uart 
 * */

#define _DMA_TX_BUF_SIZE 128
#define _DMA_RX_BUF_SIZE 128


static uint8_t tx_vfifo_buff[MICROPY_HW_MAX_UART][ _DMA_TX_BUF_SIZE ] \
                   __attribute__((section(".noncached_zidata")));
static uint8_t rx_vfifo_buff[MICROPY_HW_MAX_UART][ _DMA_RX_BUF_SIZE ] \
                   __attribute__((section(".noncached_zidata")));
/**
 * *@brief  Get the current time with the unit of millisecond.
 * *@param None.
 * *@return In this function we return current time with the unit of millisecond.
 * */
uint32_t get_current_millisecond(void)
{
    hal_gpt_status_t ret;
    uint32_t count = 0;
    uint32_t time, time_s, time_ms;
    ret = hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &count);

    if (ret != HAL_GPT_STATUS_OK) {
        return 0;
    }

    time_s = count / 32768;
    time_ms = ((count % 32768) * 1000 + 16384) / 32768;
    time = time_s * 1000 + time_ms;

    return time;
}

static void _hal_uart0_callback_event(hal_uart_callback_event_t evt, void *user_data){
    if(evt == HAL_UART_EVENT_READY_TO_READ){
        uint8_t ch = 0;
        uint8_t len = 0;
        while(true){
            len = hal_uart_receive_dma(HAL_UART_0, &ch, 1);
            if(0 == len){
                break;
            }else if (ch == mp_interrupt_char){
                mp_keyboard_interrupt();
            }else{
                ringbuf_put(&stdin_ringbuf, ch);
            }
        }
    }else if (evt == HAL_UART_EVENT_READY_TO_WRITE){
        ;// TODO WRIENT Handle!
    }else if (evt == HAL_UART_EVENT_TRANSACTION_ERROR){
        ;// TODO ERROR handle !
    }else{
        ;// TODO Unknown ERROR handle !
    }
}

/*
 * uart init
 * */

void uart_init0(void) {
	hal_uart_config_t uart_config;
    /* Set Pinmux to UART */
    hal_pinmux_set_function(HAL_GPIO_0, HAL_GPIO_0_UART1_RTS_CM4);
    hal_pinmux_set_function(HAL_GPIO_1, HAL_GPIO_1_UART1_CTS_CM4);
    hal_pinmux_set_function(HAL_GPIO_2, HAL_GPIO_2_UART1_RX_CM4);
    hal_pinmux_set_function(HAL_GPIO_3, HAL_GPIO_3_UART1_TX_CM4);

    uart_config.baudrate = HAL_UART_BAUDRATE_115200;
    uart_config.word_length = HAL_UART_WORD_LENGTH_8;
    uart_config.stop_bit = HAL_UART_STOP_BIT_1;
    uart_config.parity = HAL_UART_PARITY_NONE;
    hal_uart_init(HAL_UART_0, &uart_config);
	hal_uart_dma_config_t dma_config;

    dma_config.receive_vfifo_alert_size = 50;
    dma_config.receive_vfifo_buffer = rx_vfifo_buff[PYB_UART_0];
    dma_config.receive_vfifo_buffer_size = _DMA_RX_BUF_SIZE;
    dma_config.receive_vfifo_threshold_size = 32;

    dma_config.send_vfifo_buffer = tx_vfifo_buff[PYB_UART_0];
    dma_config.send_vfifo_buffer_size = _DMA_TX_BUF_SIZE;
    dma_config.send_vfifo_threshold_size = 32;
    hal_uart_set_dma(HAL_UART_0, &dma_config);
    hal_uart_register_callback(HAL_UART_0, _hal_uart0_callback_event, NULL);
}

bool uart_exists(int uart_id) {
    if (uart_id > MP_ARRAY_SIZE(MP_STATE_PORT(pyb_uart_obj_all))) {
        // safeguard against pyb_uart_obj_all array being configured too small
        return false;
    }
    switch (uart_id) {
        case PYB_UART_0: return true;
        case PYB_UART_1: return true;
    }
    return false;
}

// assumes Init parameters have been set up correctly
bool uart_init(pyb_uart_obj_t *uart_obj,
    uint32_t baudrate, uint32_t bits, uint32_t parity, uint32_t stop, bool flow) {
	hal_uart_config_t uart_config;
    int baudrate_enum = HAL_UART_BAUDRATE_115200;
    int bits_enum = HAL_UART_WORD_LENGTH_8;
    int parity_enum = parity; 
    int uart_port;

    switch (baudrate) {
        case 110:
            baudrate_enum = HAL_UART_BAUDRATE_110;
            break;
        case 300:
            baudrate_enum = HAL_UART_BAUDRATE_300;
            break;
        case 1200:
            baudrate_enum = HAL_UART_BAUDRATE_1200;
            break;
        case 2400:
            baudrate_enum = HAL_UART_BAUDRATE_2400;
            break;
        case 4800:
            baudrate_enum = HAL_UART_BAUDRATE_4800;
            break;
        case 9600:
            baudrate_enum = HAL_UART_BAUDRATE_9600;
            break;
        case 19200:
            baudrate_enum = HAL_UART_BAUDRATE_19200;
            break;
        case 38400:
            baudrate_enum = HAL_UART_BAUDRATE_38400;
            break;
        case 57600:
            baudrate_enum = HAL_UART_BAUDRATE_57600;
            break;
        case 115200:
            baudrate_enum = HAL_UART_BAUDRATE_115200;
            break;
        case 230400:
            baudrate_enum = HAL_UART_BAUDRATE_230400;
            break;
        case 460800:
            baudrate_enum = HAL_UART_BAUDRATE_460800;
            break;
        case 921600:
            baudrate_enum = HAL_UART_BAUDRATE_921600;
            break;
    }

    switch (bits) {
        case 5:
            bits_enum = HAL_UART_WORD_LENGTH_5;
            break;
        case 6:
            bits_enum = HAL_UART_WORD_LENGTH_6;
            break;
        case 7:
            bits_enum = HAL_UART_WORD_LENGTH_7;
            break;
        case 8:
            bits_enum = HAL_UART_WORD_LENGTH_8;
            break;
    }
    
    switch (uart_obj->uart_id) {
        case PYB_UART_1:
            uart_port = 1;
            hal_pinmux_set_function(HAL_GPIO_36, HAL_GPIO_36_UART2_RX_CM4);
            hal_pinmux_set_function(HAL_GPIO_37, HAL_GPIO_37_UART2_TX_CM4);
            break;

        default:
            // UART does not exist or is not configured for this board
            return false;
    }

    uart_config.baudrate = baudrate_enum;
    uart_config.word_length = bits_enum;
    uart_config.stop_bit = stop;
    uart_config.parity = parity_enum;
    hal_uart_init(uart_obj->uart_id, &uart_config);

    uart_obj->baudrate = baudrate;
    uart_obj->is_enabled = true;
    uart_obj->attached_to_repl = false;
    uart_obj->char_width = bits ;
    uart_obj->send_notice = false;
    uart_obj->receive_notice = false;
    uart_obj->event_transaction_error = false;
    uart_obj->parity = parity_enum;
    uart_obj->stop = stop;
    uart_obj->is_flow = flow;
    uart_obj->is_any = false;

    return true;
}

void uart_set_rxbuf(pyb_uart_obj_t *self, size_t len, void *buf) {
    if (self->uart_id == PYB_UART_1) {
        self->read_buf_head = 0;
        self->read_buf_tail = 0;
        self->read_buf_len = len;
        self->read_buf = buf;
    }
}

int uart_deinit(pyb_uart_obj_t *self) {
    self->is_enabled = false;
    int err = -1;
    if (self->uart_id < PYB_UART_0 || self->uart_id >= PYB_UART_MAX) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) does not exist", self->uart_id));
    }

    // Reset and turn off the UART peripheral
    if (self->uart_id == PYB_UART_0) {
       nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) is disabled (dedicated to REPL)", self->uart_id));
    } else if (self->uart_id == PYB_UART_1) {
        err = hal_uart_deinit(HAL_UART_1);        
    }
    return err;
}

void uart_attach_to_repl(pyb_uart_obj_t *self, bool attached) {
    self->attached_to_repl = attached;
}

uint32_t uart_get_baudrate(pyb_uart_obj_t *self) {
    return self->baudrate;
}

// assumes there is a character available
uint32_t uart_rx_char(pyb_uart_obj_t *self) {
    uint32_t value;
    uint32_t time1;
    uint32_t time2;
    self->receive_notice = false;
    self->event_transaction_error = false; 
    self->is_any = true;
    time1 = get_current_millisecond();

    while (1) {
        value = hal_uart_get_char_unblocking(self->uart_id);
        if (value != 0xffffffff){ 
            break;
        }

        time2 = get_current_millisecond();
        if ((time2 - time1) >= self->timeout){
            self->is_any = false;
            return -1;
        }
    }
    self->is_any = false;
    return value &= 0xff;
}

mp_uint_t uart_rx_any(pyb_uart_obj_t *self) {
    if (self->is_any)
        return 1;
    return -1;
}


// src - a pointer to the data to send (16-bit aligned for 9-bit chars)
// num_chars - number of characters to send (9-bit chars count for 2 bytes from src)
// *errcode - returns 0 for success, MP_Exxx on error
// returns the number of characters sent (valid even if there was an error)
size_t uart_tx_data(pyb_uart_obj_t *self, const void *src_in, size_t num_chars, int *errcode) {
    if (num_chars == 0) {
        *errcode = 0;
        return 0;
    }

    char *src = (char*)src_in;
    size_t num_tx = 0;
    int sed_cnt  = 0;
    uint32_t sed_size = (uint32_t *)num_chars;
    uint32_t time1;
    uint32_t time2;
    self->send_notice = false;
    self->event_transaction_error = false;
    time1 = get_current_millisecond();
    while (1) {
        sed_cnt = hal_uart_send_polling(self->uart_id, src, sed_size);
        src += sed_cnt;
        sed_size -= sed_cnt;
        if (sed_size == 0)
            break;
        else if (sed_cnt > 0) {
            time1 = get_current_millisecond();
        }
        time2 = get_current_millisecond();
        if ((time2 - time1) >= self->timeout){
            *errcode = MP_ETIMEDOUT;
            return num_tx;
        }
    }

    *errcode = 0;
    return num_tx;
}

void uart_tx_strn(pyb_uart_obj_t *uart_obj, const char *str, uint len) {
    int errcode;
    uart_tx_data(uart_obj, str, len, &errcode);
}

