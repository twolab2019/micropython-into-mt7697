/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2018 Damien P. George
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

#include <stdint.h>
#include <stdio.h>

#include "py/obj.h"
#include "py/mperrno.h"
#include "hal_cache.h"
#include "flash_map.h"
#include "hal_flash.h"
#include "storage.h"

#if MICROPY_HW_ENABLE_INTERNAL_FLASH_STORAGE

// Here we try to automatically configure the location and size of the flash
// pages to use for the internal storage.  We also configure the location of the
// cache used for writing.



#define FLASH_FLAG_DIRTY        (1)
#define FLASH_FLAG_FORCE_WRITE  (2)
#define FLASH_FLAG_ERASED       (4)
// -- depending mt7687_flash.ld file FLASH_FS 
// mt7687_flash.ld: FLASH_FS : 408 * 1024 / 512 = 816
#define FLASH_MEM_SEG1_NUM_BLOCKS (816)
#define FLASH_MEM_SEG1_START_ADDR HAL_FLASH_FS_START
#define FLASH_BUFF_SIZE 4096

static void flash_bdev_irq_handler(void);

uint8_t _flash_buff[FLASH_BUFF_SIZE];

int32_t flash_bdev_ioctl(uint32_t op, uint32_t arg) {
    (void)arg;
    switch (op) {
        case BDEV_IOCTL_INIT:
            // -- TODO: initial the cache and flash? 
            hal_flash_init();
            return 0;

        case BDEV_IOCTL_NUM_BLOCKS:
            return FLASH_MEM_SEG1_NUM_BLOCKS;

        case BDEV_IOCTL_IRQ_HANDLER:
            // mt7697 has no flash irq
            // this case if useless !
            flash_bdev_irq_handler();
            return 0;

        case BDEV_IOCTL_SYNC: {
            flash_bdev_irq_handler();
            return 0;
        }
    }
    return -MP_EINVAL;
}


static uint32_t convert_block_to_flash_addr(uint32_t block) {
    if (block < FLASH_MEM_SEG1_NUM_BLOCKS) {
		uint32_t flash_addr = FLASH_MEM_SEG1_START_ADDR + block * FLASH_BLOCK_SIZE;
		if(flash_addr < HAL_FLASH_FS_END){
			return flash_addr;
		} else {
			printf("[FS][ERROR][address out of range][0x%08x]\r\n", flash_addr);
		}
    }
    // can add more flash segments here if needed, following above pattern

    // bad block
    return -1;
}

static void flash_bdev_irq_handler(void) {
#ifdef HAL_CACHE_MODULE_ENABLED
    hal_cache_invalidate_all_cache_lines();
#endif
}

bool flash_bdev_readblock(uint8_t *dest, uint32_t block) {
    uint32_t flash_addr = convert_block_to_flash_addr(block);
    
	if(HAL_FLASH_STATUS_OK != hal_flash_read(flash_addr, dest, FLASH_BLOCK_SIZE)){
		printf("[hal flash read][flash bdev readblock][error]\r\n");
		return false;
	}
    return true;
}
bool flash_bdev_writeblock(const uint8_t *src, uint32_t block) {
    uint32_t flash_addr = convert_block_to_flash_addr(block);
	uint32_t base_addr_start = flash_addr & ~((uint32_t)0xfff);
	uint32_t index_start = flash_addr - base_addr_start;
	uint32_t i = 0;

	if(flash_addr == -1){
		// bad block number
		return false;
	}

	if(HAL_FLASH_STATUS_OK != hal_flash_read(base_addr_start, _flash_buff, FLASH_BUFF_SIZE)){
		printf("[hal flash read][error]\r\n");
		return false;
	}
	for(i = 0;i < FLASH_BLOCK_SIZE; i++){
		_flash_buff[index_start + i] = *(src+i);
	}
	if(HAL_FLASH_STATUS_OK != hal_flash_erase(base_addr_start, HAL_FLASH_BLOCK_4K)){
		printf("[hal flash erase][error]\r\n");
		return false;
	}
	if(HAL_FLASH_STATUS_OK != hal_flash_write(base_addr_start, _flash_buff, FLASH_BUFF_SIZE)){
		printf("[hal flash write][error]\r\n");
		return false;
	}

	return true;
}

#endif // MICROPY_HW_ENABLE_INTERNAL_FLASH_STORAGE
