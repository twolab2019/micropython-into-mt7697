#ifndef MPHALPORT_H
#define MPHALPORT_H
#include "py/mpconfig.h"
#include "machine_pin.h"
#include "hal.h"
#include "py/ringbuf.h"

extern ringbuf_t stdin_ringbuf;
static inline mp_uint_t mp_hal_ticks_ms(void) { return 0; }
uint32_t get_current_millisecond(void);

/* GPIO function porting */
#define mp_hal_pin_obj_t        const machine_pin_obj_t*
#define mp_hal_pin_name(o)      (o->pin_num)
#define mp_hal_pin_input(p)     hal_gpio_set_direction(p->gpio_num, HAL_GPIO_DIRECTION_INPUT)
#define mp_hal_pin_output(p)    hal_gpio_set_direction(p->gpio_num, HAL_GPIO_DIRECTION_OUTPUT)
#define mp_hal_pin_read(p)      _mp_hal_pin_read(p->gpio_num)
#define mp_hal_pin_write(p,v)   _mp_hal_pin_write(p->gpio_num, v)
#define mp_hal_get_pin_obj(o)   machine_pin_obj_from_upy_obj(o)

#define MP_HAL_PIN_FMT "%u"
#define mp_hal_delay_us_fast(p) hal_gpt_delay_us(p)
#endif
