
#include "FreeRTOS.h"
#include "task.h"
#include "hal.h"
#include "sys_init.h"

#include "py/ringbuf.h"
#include "py/mpthread.h"
#include "py/mpstate.h"
#include "py/mphal.h"
#include "mphalport.h"
#include "lib/utils/interrupt_char.h"

/* onionys
 * set up which uart port
 * mt7697 uart port 0 : gpio 2,3
 *             port 1 : gpio 37,36
 * */

#define MY_UART_PORT HAL_UART_0 

/**
 * *@brief  Get the current time with the unit of millisecond.
 * *@param None.
 * *@return In this function we return current time with the unit of millisecond.
 * */
uint32_t get_current_millisecond(void)
{
    hal_gpt_status_t ret;
    uint32_t count = 0;
    uint32_t time, time_s, time_ms;
    ret = hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &count);

    if (ret != HAL_GPT_STATUS_OK) {
        return 0;
    }

    time_s = count / 32768;
    time_ms = ((count % 32768) * 1000 + 16384) / 32768;
    time = time_s * 1000 + time_ms;

    return time;
}

static uint8_t stdin_ringbuf_array[256];
ringbuf_t stdin_ringbuf = {stdin_ringbuf_array, sizeof(stdin_ringbuf_array)};

int mp_hal_stdin_rx_chr(void){
	for(;;){
		int c = 0;
		c = ringbuf_get(&stdin_ringbuf);
		if (c == -1) {
			MICROPY_EVENT_POLL_HOOK;
			ulTaskNotifyTake(pdFAIL,1);
		}else{
			return c;
		}
	}
}

void mp_hal_stdout_tx_char(char c){
	hal_uart_put_char(MY_UART_PORT,c);
}

void mp_hal_stdout_tx_str(const char * str){
	MP_THREAD_GIL_EXIT();
	while(*str){
		mp_hal_stdout_tx_char(*str++);
	}
	MP_THREAD_GIL_ENTER();
}

void mp_hal_stdout_tx_strn(const char *str, size_t len){
	MP_THREAD_GIL_EXIT();
	while(len--){
		mp_hal_stdout_tx_char(*str++);
	}
	MP_THREAD_GIL_ENTER();
}

void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len){
	MP_THREAD_GIL_EXIT();
	while(len--){
		if(*str == '\n'){
			hal_uart_put_char(MY_UART_PORT, '\r');
		}
		hal_uart_put_char(MY_UART_PORT, *str++);
	}
	MP_THREAD_GIL_ENTER();
}


/*
 * port for i2c
 * CAUTION ! : 7697 output has no open-drain mode
 * */

void mp_hal_pin_od_low(mp_hal_pin_obj_t pin){
	mp_hal_pin_write(pin,0);
	mp_hal_pin_output(pin);
}

void mp_hal_pin_od_high(mp_hal_pin_obj_t pin){
	mp_hal_pin_input(pin);
}
void mp_hal_pin_open_drain(mp_hal_pin_obj_t pin){
	hal_gpio_disable_pull(pin->gpio_num);
}
