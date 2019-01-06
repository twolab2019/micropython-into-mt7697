
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
 * config uart 
 * */

#define _DMA_TX_BUF_SIZE 128
#define _DMA_RX_BUF_SIZE 128
static uint8_t tx_vfifo_buff[ _DMA_TX_BUF_SIZE ] __attribute__((section(".noncached_zidata")));
static uint8_t rx_vfifo_buff[ _DMA_RX_BUF_SIZE ] __attribute__((section(".noncached_zidata")));
static void _hal_uart_receive_callback(hal_uart_callback_event_t evt, void *user_data){
    if(evt == HAL_UART_EVENT_READY_TO_READ){
        uint8_t ch = 0;
        uint8_t len = 0;
        while(true){
            len = hal_uart_receive_dma(HAL_UART_0, &ch, 1);
            if(0 == len){
                break;
            }else if (ch == mp_interrupt_char){
                mp_keyboard_interrupt();
            }else{
                ringbuf_put(&stdin_ringbuf, ch);
            }
        }
    }else if (evt == HAL_UART_EVENT_READY_TO_WRITE){
        ;// TODO WRIENT Handle!
    }else if (evt == HAL_UART_EVENT_TRANSACTION_ERROR){
        ;// TODO ERROR handle !
    }else{
        ;// TODO Unknown ERROR handle !
    }
}

/*
 * uart init
 * */

int hw_uart_init(){
	hal_uart_config_t uart_config;
    /* Set Pinmux to UART */
    hal_pinmux_set_function(HAL_GPIO_0, HAL_GPIO_0_UART1_RTS_CM4);
    hal_pinmux_set_function(HAL_GPIO_1, HAL_GPIO_1_UART1_CTS_CM4);
    hal_pinmux_set_function(HAL_GPIO_2, HAL_GPIO_2_UART1_RX_CM4);
    hal_pinmux_set_function(HAL_GPIO_3, HAL_GPIO_3_UART1_TX_CM4);

    uart_config.baudrate = HAL_UART_BAUDRATE_115200;
    uart_config.word_length = HAL_UART_WORD_LENGTH_8;
    uart_config.stop_bit = HAL_UART_STOP_BIT_1;
    uart_config.parity = HAL_UART_PARITY_NONE;
    hal_uart_init(HAL_UART_0, &uart_config);
	hal_uart_dma_config_t dma_config;

    dma_config.receive_vfifo_alert_size = 50;
    dma_config.receive_vfifo_buffer = rx_vfifo_buff;
    dma_config.receive_vfifo_buffer_size = _DMA_RX_BUF_SIZE;
    dma_config.receive_vfifo_threshold_size = 32;

    dma_config.send_vfifo_buffer = tx_vfifo_buff;
    dma_config.send_vfifo_buffer_size = _DMA_TX_BUF_SIZE;
    dma_config.send_vfifo_threshold_size = 32;
    hal_uart_set_dma(HAL_UART_0, &dma_config);
    hal_uart_register_callback(HAL_UART_0, _hal_uart_receive_callback, NULL);
	return 0;
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
