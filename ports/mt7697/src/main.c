/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* device.h includes */
#include "mt7687.h"
#include "system_mt7687.h"

#include "sys_init.h"
#include "task_def.h"

#include "wifi_lwip_helper.h"
#include "wifi_api.h"

#include "bt_init.h"
#include "storage.h"
//#include "mphalport.h"

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/stackctrl.h"
#include "lib/utils/pyexec.h"

// -- fs fat
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
#include "storage.h"

fs_user_mount_t fs_user_mount_flash;

// -- 16k bytes for stack size of freeRTOS task--
#define MP_TASK_STACK_SIZE    (20 * 1024)
#define MP_TASK_STACK_LEN     (MP_TASK_STACK_SIZE / sizeof(StackType_t))

static char heap[50*1024];
static char *stack_top;
extern uint32_t __StackTop;
extern uint32_t __StackLimit;

/* Create the log control block as user wishes. Here we use 'template' as module name.
 * User needs to define their own log control blocks as project needs.
 * Please refer to the log dev guide under /doc folder for more details.
 */
log_create_module(template, PRINT_LEVEL_INFO);

#if MICROPY_HW_ENABLE_STORAGE
static const char fresh_boot_py[] =
"# boot.py -- run on boot-up\r\n"
"# can run arbitrary Python, but best to keep it minimal\r\n"
"\r\n"
#if MICROPY_HW_ENABLE_USB
"#pyb.usb_mode('VCP+MSC') # act as a serial and a storage device\r\n"
"#pyb.usb_mode('VCP+HID') # act as a serial device and a mouse\r\n"
#endif
;

static const char fresh_main_py[] =
"# main.py -- put your code here!\r\n"
;

static const char fresh_readme_txt[] =
"This is a Liniit 7697 HDK board with MicroPython.\r\n"
" Author : onionys@gmail.com and tomlin690@gmail.com\r\n"
"\r\n"
"You can get started right away by writing your Python code in 'main.py'.\r\n"
"\r\n"
;

// avoid inlining to avoid stack usage within main()
MP_NOINLINE STATIC bool init_flash_fs(uint reset_mode) {
    // init the vfs object
	LOG_I(template, "[fs init][mode][%d]", reset_mode);
    fs_user_mount_t *vfs_fat = &fs_user_mount_flash;
    vfs_fat->flags = 0;
    pyb_flash_init_vfs(vfs_fat);
	LOG_I(template, "[fs mount]");

    // try to mount the flash
    FRESULT res = f_mount(&vfs_fat->fatfs);

    if (reset_mode == 3 || res == FR_NO_FILESYSTEM) {
        // no filesystem, or asked to reset it, so create a fresh one
        LOG_I(template, "[fs][fs not found!]");

        uint8_t working_buf[_MAX_SS];
        LOG_I(template, "[fs][low level operation][mkfs]");
        res = f_mkfs(&vfs_fat->fatfs, FM_FAT, 0, working_buf, sizeof(working_buf));
        if (res == FR_OK) {
            // success creating fresh LFS
			printf("[fs][success create frsh LFS]\r\n");
        } else {
            printf("PYB: can't create flash filesystem\n");
            return false;
        }

        // set label
        f_setlabel(&vfs_fat->fatfs, MICROPY_HW_FLASH_FS_LABEL);

        // create empty main.py
        FIL fp;
        f_open(&vfs_fat->fatfs, &fp, "/main.py", FA_WRITE | FA_CREATE_ALWAYS);
        UINT n;
        f_write(&fp, fresh_main_py, sizeof(fresh_main_py) - 1 /* don't count null terminator */, &n);
        // TODO check we could write n bytes
        f_close(&fp);
		LOG_I(template, "[fs file][create main.py]");

        // create readme file
        f_open(&vfs_fat->fatfs, &fp, "/README.txt", FA_WRITE | FA_CREATE_ALWAYS);
        f_write(&fp, fresh_readme_txt, sizeof(fresh_readme_txt) - 1 /* don't count null terminator */, &n);
        f_close(&fp);
		LOG_I(template, "[fs file][create README]");
    } else if (res == FR_OK) {
        // mount sucessful
		printf("[fs][mount ok!]\r\n");
    } else {
    fail:
        printf("PYB: can't mount flash\n");
        return false;
    }

    // mount the flash device (there should be no other devices mounted at this point)
    // we allocate this structure on the heap because vfs->next is a root pointer
    mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);
    if (vfs == NULL) {
		LOG_I(template, "[vfs fail]");
        goto fail;
    }
    vfs->str = "/flash";
    vfs->len = 6;
    vfs->obj = MP_OBJ_FROM_PTR(vfs_fat);
    vfs->next = NULL;
    MP_STATE_VM(vfs_mount_table) = vfs;

    // The current directory is used as the boot up directory.
    // It is set to the internal flash filesystem by default.
    MP_STATE_PORT(vfs_cur) = vfs;

    // Make sure we have a /flash/boot.py.  Create it if needed.
    FILINFO fno;
    res = f_stat(&vfs_fat->fatfs, "/boot.py", &fno);
    if (res != FR_OK) {
        // doesn't exist, create fresh file
		LOG_I(template, "[fs file][create /boot.py]");
        FIL fp;
        f_open(&vfs_fat->fatfs, &fp, "/boot.py", FA_WRITE | FA_CREATE_ALWAYS);
        UINT n;
        f_write(&fp, fresh_boot_py, sizeof(fresh_boot_py) - 1 /* don't count null terminator */, &n);
        // TODO check we could write n bytes
        f_close(&fp);
    }

    return true;
}
#endif

void mp_task(void *pvParameters)
{

    #if MICROPY_HW_ENABLE_STORAGE
    storage_init();
    #endif

    #if MICROPY_ENABLE_GC

    // Stack limit should be less than real stack size, so we have a chance
    // to recover from limit hit.  (Limit is measured in bytes.)
    // Note: stack control relies on main thread being initialised above
    mp_stack_set_top(&__StackTop);
    mp_stack_set_limit((char*)&__StackTop - (char*)&__StackLimit - 1024);

    // GC init
    gc_init(heap, heap + sizeof(heap));
    #endif
    SysInitStatus_Set();

    // MicroPython init
    mp_init();
    mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_path), 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)
    readline_init0();

    // Initialise the local flash filesystem.
    // Create it if needed, mount in on /flash, and set it as current dir.
    bool mounted_flash = false;
    #if MICROPY_HW_ENABLE_STORAGE
	uint reset_mode = 0;
    mounted_flash = init_flash_fs(reset_mode);
    #endif

    mp_hal_stdout_tx_str("-- mp_task --");
    #if MICROPY_ENABLE_COMPILER
    #if MICROPY_REPL_EVENT_DRIVEN
    pyexec_event_repl_init();
    for (;;) {
        int c = mp_hal_stdin_rx_chr();
        if (pyexec_event_repl_process_char(c)) {
            break;
        }
    }
    #else
    pyexec_friendly_repl();
    #endif

    #else
        pyexec_frozen_module("frozentest.py");
    #endif

}


/*
* @brief       Main function
* @param[in]   None.
* @return      None.
*/
int main(void)
{
    /* Do system initialization, eg: hardware, nvdm and random seed. */
    TaskHandle_t task_one = NULL;
    BaseType_t task_res;
    system_init();

    /* system log initialization.
     * This is the simplest way to initialize system log, that just inputs three NULLs
     * as input arguments. User can use advanved feature of system log along with NVDM.
     * For more details, please refer to the log dev guide under /doc folder or projects
     * under project/mtxxxx_hdk/apps/.
     */
    log_init(NULL, NULL, NULL);
    LOG_I(template, "start to create task. 5\n");
   // SysInitStatus_Set();
    int stack_dummy;
    stack_top = (char*)&stack_dummy;
    task_res = xTaskCreate(
		mp_task,
		"mp_task",
		MP_TASK_STACK_LEN,
		(void *) 1,
		TASK_PRIORITY_LOW,
		&task_one);

    if (task_res == pdPASS)
        mp_hal_stdout_tx_str("\n[MP Task OK]\n");
    else
        mp_hal_stdout_tx_str("\n[MP Task init failed]\n");

    vTaskStartScheduler();
    mp_deinit();
    return 0;

}

void nlr_jump_fail(void *val) {
    while (1);
}

void NORETURN __fatal_error(const char *msg) {
    while (1);
}
