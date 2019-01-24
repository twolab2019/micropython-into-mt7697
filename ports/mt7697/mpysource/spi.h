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
#define SPI_TEST_FREQUENCY             1000000
#define SPI_TEST_MASTER                HAL_SPI_MASTER_0
#define SPI_TEST_MASTER_SLAVE          HAL_SPI_MASTER_SLAVE_0
#define SPIM_PIN_NUMBER_CS             HAL_GPIO_32
#define SPIM_PIN_FUNC_CS               HAL_GPIO_32_SPI_CS_0_M_CM4
#define SPIM_PIN_NUMBER_CLK            HAL_GPIO_31
#define SPIM_PIN_FUNC_CLK              HAL_GPIO_31_SPI_SCK_M_CM4
#define SPIM_PIN_NUMBER_MOSI           HAL_GPIO_30
#define SPIM_PIN_FUNC_MOSI             HAL_GPIO_30_SPI_MISO_M_CM4
#define SPIM_PIN_NUMBER_MISO           HAL_GPIO_29
#define SPIM_PIN_FUNC_MISO             HAL_GPIO_29_SPI_MOSI_M_CM4

#define SPI_TEST_DATA_SIZE             (20 * 1024)
#define SPI_TEST_TX_DATA_PATTERN       0x01234567
#define SPI_TEST_RX_DATA_PATTERN       0x89abcdef
#define SPI_TEST_SLV_BUFFER_ADDRESS    0x20000200  /* SPI slave fix this buffer address according to linker script. */
#define SPI_TEST_CONTROL_SIZE          4
#define SPI_TEST_STATUS_SIZE           4
#define SPI_TEST_READ_DATA_BEGIN       0x0A
#define SPI_TEST_READ_DATA_END         0xA0
#define SPI_TEST_WRITE_DATA_BEGIN      0x55
#define SPI_TEST_WRITE_DATA_END        0xAA
#define SPI_TEST_ACK_FLAG              0x5A
#define SPI_SLAVER_REG_READ_LOW_DATA   0x00
#define SPI_SLAVER_REG_READ_HIGH_DATA  0x02
#define SPI_SLAVER_REG_WRITE_LOW_DATA  0x04
#define SPI_SLAVER_REG_WRITE_HIGH_DATA 0x06
#define SPI_SLAVER_REG_LOW_ADDRESS     0x08
#define SPI_SLAVER_REG_HIGH_ADDRESS    0x0A
#define SPI_SLAVER_REG_CONFIG          0x0C
#define SPI_SLAVER_REG_STATUS          0x10
#define SPI_SLAVER_REG_SLV_IRQ         0x14
#define SPI_SLAVER_REG_RCV_MAILBOX     0x18
#define SPI_SLAVER_CONFIG_BUS_SIZE     (0x02 << 1)  /* 4 word */
#define SPI_SLAVER_CONFIG_BUS_W        0x01         /* write */
#define SPI_SLAVER_CONFIG_BUS_R        0x00         /* read */
#define SPI_SLAVER_CONFIG_READ         0x00
#define SPI_SLAVER_CONFIG_WRITE        0x80

typedef struct _machine_hw_spi_obj_t {
    mp_obj_base_t base;
    bool is_master;
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

typedef struct _hw_spi_trans_slave_data {
    uint32_t *src;
    uint32_t *dest;
    size_t *len;
} hw_spi_trans_slave_data;
extern  uint8_t slaver_data_buffer[SPI_TEST_DATA_SIZE + SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE];
extern  volatile bool transfer_data_finished;
extern const mp_obj_type_t machine_hw_spi_type;

extern void spim_wait_slave_idle(void);
extern void spim_write_slave_address(uint32_t address);
extern void spim_write_slave_command(bool is_read);
extern void spim_write_slave_data(uint32_t data);
extern uint32_t spim_read_slave_data(void);
extern void spim_wait_slave_ack(void);
extern void spim_active_slave_irq(void);
extern void spis_user_callback(void *from_upy_data);
#endif 
