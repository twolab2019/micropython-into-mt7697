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

#ifndef MICROPY_INCLUDED_MT7697_HW_SPI_H
#define MICROPY_INCLUDED_MT7697_HW_SPI_H
/* hal includes */
#include "hal.h"
#include "system_mt7687.h"
#include "top.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define SPIM_PIN_NUMBER_CS      HAL_GPIO_32
#define SPIM_PIN_FUNC_CS        HAL_GPIO_32_GPIO32
#define SPIM_PIN_NUMBER_CLK     HAL_GPIO_31
#define SPIM_PIN_FUNC_CLK       HAL_GPIO_31_SPI_SCK_M_CM4
#define SPIM_PIN_NUMBER_MOSI    HAL_GPIO_29
#define SPIM_PIN_FUNC_MOSI      HAL_GPIO_29_SPI_MOSI_M_CM4
#define SPIM_PIN_NUMBER_MISO    HAL_GPIO_30
#define SPIM_PIN_FUNC_MISO      HAL_GPIO_30_SPI_MISO_M_CM4
typedef struct _machine_hw_spi_obj_t {
    mp_obj_base_t base;
    uint8_t  id;
    uint32_t baudrate;
    uint8_t polarity;
    uint8_t phase;
    uint8_t bits;
    uint8_t firstbit;
    int8_t sck;
    int8_t mosi;
    int8_t miso;
    enum {
        MACHINE_HW_SPI_STATE_NONE,
        MACHINE_HW_SPI_STATE_INIT,
        MACHINE_HW_SPI_STATE_DEINIT
    } state;
    uint32_t address;
} machine_hw_spi_obj_t;

extern const mp_obj_type_t machine_hw_spi_type;
#endif 
