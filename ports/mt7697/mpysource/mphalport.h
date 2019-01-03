#ifndef MPHALPORT_H
#define MPHALPORT_H
#include "py/mpconfig.h"
#include "py/ringbuf.h"

extern ringbuf_t stdin_ringbuf;
static inline mp_uint_t mp_hal_ticks_ms(void) { return 0; }
#endif
