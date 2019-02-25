/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018-2019 "Song Yang" <onionys@gmail.com>
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
#include "py/builtin.h"

const char mt7697hdk_help_text[] =
"Welcome to MicroPython on the linkit7697 HDK!\n"
"\n"
"For generic online docs please visit http://docs.micropython.org/\n"
"\n"
" linkit 7697 HDK Pin map \n"
"\n"
"                 /--------------------\\                           \n"
"                 |* GND          3V3 *|                            \n"
"       UART0_RX--|* P0           P6  *|--UART1_TX-EINT20-(USR-BTN) \n"
" EINT2-UART0_TX--|* P1           P7  *|--UART1_RX--------(USR-LED) \n"
" EINT0-----------|* P2           P8  *|--HW_I2C1_CLK               \n"
" EINT22----------|* P3           P9  *|--HW_I2C1_DATA              \n"
"          IR_RX--|* P4           P10 *|--HW_SPI_CS0                \n"
"          IR_TX--|* P5           P11 *|--HW_SPI_MOSI               \n"
"                 |* GND          P12 *|--HW_SPI_MISO               \n"
"      (RST-BTN)--|* RST          P13 *|--HW_SPI_SCK                \n"
"                 |* 3V3          P14 *|--ADC_IN0                   \n"
"                 |* 5V           P15 *|--ADC_IN1                   \n"
"                 |* D-           P16 *|--ADC_IN2                   \n"
"                 |* D+           P17 *|--ADC_IN3                   \n"
"                 |* GND          GND *|                            \n"
"                 | [RST-BTN] [USR-BTN]|\n"
"                 |       /-----\\      |\n"
"                 |       |     |      |\n"
"                 ----------------------\n"
"                           USB         \n"
" PWM: P0~P17 \n"
"\n"
"Control commands:\n"
"  CTRL-A        -- on a blank line, enter raw REPL mode\n"
"  CTRL-B        -- on a blank line, enter normal REPL mode\n"
"  CTRL-C        -- interrupt a running program\n"
"  CTRL-D        -- on a blank line, do a soft reset of the board\n"
"  CTRL-E        -- on a blank line, enter paste mode\n"
"\n"
"For further help on a specific object, type help(obj)\n"
"For a list of available modules, type help('modules')\n"
;
