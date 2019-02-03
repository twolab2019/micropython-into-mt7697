#ifndef __LINKIT_IT_7697_MODMACHINE_H__
#define __LINKIT_IT_7697_MODMACHINE_H__

#include "py/obj.h"

void machine_init(void);
void machine_deinit(void);

// extern const mp_obj_type_t machine_pwm_type;
extern const mp_obj_type_t machine_adc_type;
// extern const mp_obj_type_t machine_rtc_type;
extern const mp_obj_type_t machine_pin_type;
// extern const mp_obj_type_t machine_timer_type;
extern const mp_obj_type_t machine_i2c_type;
extern const mp_obj_type_t machine_wdt_type;
// extern const mp_obj_type_t machine_uart_type;

#endif 
