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

#include "hal.h"

#include "py/runtime.h"
#include "lib/oofatfs/ff.h"

#include "systick.h"
#include "machine_rtc.h"


DWORD get_fattime(void) {
    hal_rtc_time_t now;
    if(HAL_RTC_STATUS_OK != hal_rtc_get_time(&now)){
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "HAL RTC gets time error!!"));
    }
    return ( (uint32_t) (_RTC_BASE_YEAR + now.rtc_year - 1980) << 25 ) |
           ( (uint32_t) now.rtc_mon  << 21 ) |
           ( (uint32_t) now.rtc_day  << 16 ) |
           ( (uint32_t) now.rtc_hour << 11)  |
           ( (uint32_t) now.rtc_min  <<  5)  |
           ( (uint32_t) now.rtc_sec   / 2 )  ;

}
