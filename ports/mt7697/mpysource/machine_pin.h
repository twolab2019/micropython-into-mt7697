#ifndef __LINKIT_7697_MICROPYTHON_MACHINE_PIN_H__
#define __LINKIT_7697_MICROPYTHON_MACHINE_PIN_H__

#include "hal.h"
#include "py/obj.h"

/* for class constant define */
#define GPIO_PIN_NONE 0
#define	GPIO_PIN_DIR_INPUT 0
#define	GPIO_PIN_DIR_OUTPUT 1
#define	GPIO_PIN_DIR_OPEN_DRAIN 3
#define	GPIO_PIN_PULL_DOWN 0
#define	GPIO_PIN_PULL_UP 1
#define	GPIO_PIN_PULL_DISABLE 2

#define	GPIO_PIN_INTR_LO_LEVEL 0
#define	GPIO_PIN_INTR_HI_LEVEL 1

typedef struct _machine_pin_obj_t {
	mp_obj_base_t base;
	hal_gpio_pin_t gpio_num;
	uint8_t pin_num; // for linkit 7697 P# (PIN NUM)
	uint8_t gpio_pinmux_num;
	hal_pwm_channel_t pwm_ch;
	uint8_t pwm_pinmux_num;
	uint8_t eint_pinmux_num;
	hal_eint_number_t eint_num;
	hal_adc_channel_t adc_ch;
} machine_pin_obj_t;

extern const mp_obj_type_t machine_pin_type;

void machine_pins_init(void);
void machine_pins_deinit(void);

const machine_pin_obj_t * machine_pin_obj_from_gpio_num(hal_gpio_pin_t gpio_num);
const machine_pin_obj_t * machine_pin_obj_from_pin_num(uint8_t pin_num);
const machine_pin_obj_t * machine_pin_obj_from_upy_obj(mp_obj_t upy_obj);
uint8_t _mp_hal_pin_read(hal_gpio_pin_t gpio_num);

int _mp_hal_pin_write(uint32_t gpio_num, uint8_t val);

#endif 
