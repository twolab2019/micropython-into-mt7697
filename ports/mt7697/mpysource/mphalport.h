#ifndef MPHALPORT_H
#define MPHALPORT_H
#include "py/mpconfig.h"

static inline mp_uint_t mp_hal_ticks_ms(void) { return 0; }

int hw_uart_init();
#endif
