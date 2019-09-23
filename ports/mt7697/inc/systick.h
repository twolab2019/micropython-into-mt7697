#ifndef __LINKIT_7697_HDK_SYSTICK_H__
#define __LINKIT_7697_HDK_SYSTICK_H__
#include "mpconfigport.h"
void      mp_hal_delay_ms(mp_uint_t ms);
void      mp_hal_delay_us(mp_uint_t us);
mp_uint_t mp_hal_ticks_ms(void);
mp_uint_t mp_hal_ticks_us(void);
mp_uint_t mp_hal_ticks_cpu(void);

void hw_system_clock_config(void);

void software_reset(void);
#endif
