#include <stdint.h>
#include "mpconfigboard_common.h"

// options to control how MicroPython is built

// You can disable the built-in MicroPython compiler by setting the following
// config option to 0.  If you do this then you won't get a REPL prompt, but you
// will still be able to execute pre-compiled scripts, compiled with mpy-cross.




#define MICROPY_ENABLE_COMPILER     (1)
#define MICROPY_REPL_EVENT_DRIVEN   (1)
// memory allocation policies
#define MICROPY_ALLOC_PATH_MAX      (128)

// emitters
#define MICROPY_PERSISTENT_CODE_LOAD (1)
#ifndef MICROPY_EMIT_THUMB
#define MICROPY_EMIT_THUMB          (1)
#endif
#ifndef MICROPY_EMIT_INLINE_THUMB
#define MICROPY_EMIT_INLINE_THUMB   (0)
#endif

// compiler configuration
#define MICROPY_COMP_MODULE_CONST   (1)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (1)
#define MICROPY_COMP_RETURN_IF_EXPR (1)

// optimisations
#define MICROPY_OPT_COMPUTED_GOTO   (1)
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE (1)
#define MICROPY_OPT_MPZ_BITWISE     (1)
#define MICROPY_OPT_MATH_FACTORIAL  (1)

// Python internal features
#define MICROPY_READER_VFS          (1)
#define MICROPY_ENABLE_GC           (1)
#define MICROPY_ENABLE_FINALISER    (1)
#define MICROPY_STACK_CHECK         (0)
#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF (1)
#define MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE (1)
#define MICROPY_KBD_EXCEPTION       (1)
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_REPL_EMACS_KEYS     (1)
#define MICROPY_REPL_AUTO_INDENT    (1)
#define MICROPY_LONGINT_IMPL        (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_ENABLE_SOURCE_LINE  (1)
#ifndef MICROPY_FLOAT_IMPL // can be configured by each board via mpconfigboard.mk
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_FLOAT)
#endif
#define MICROPY_STREAMS_NON_BLOCK   (1)
#define MICROPY_MODULE_WEAK_LINKS   (1)
#define MICROPY_CAN_OVERRIDE_BUILTINS (1)
#define MICROPY_USE_INTERNAL_ERRNO  (1)
#define MICROPY_ENABLE_SCHEDULER    (1)
#define MICROPY_SCHEDULER_DEPTH     (8)
#define MICROPY_VFS                 (1)
#ifndef MICROPY_VFS_FAT
#define MICROPY_VFS_FAT             (1)
#endif

// control over Python builtins
#define MICROPY_PY_FUNCTION_ATTRS   (1)
#define MICROPY_PY_DESCRIPTORS      (1)
#define MICROPY_PY_DELATTR_SETATTR  (1)
#define MICROPY_PY_BUILTINS_STR_UNICODE (1)
#define MICROPY_PY_BUILTINS_STR_CENTER (1)
#define MICROPY_PY_BUILTINS_STR_PARTITION (1)
#define MICROPY_PY_BUILTINS_STR_SPLITLINES (1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW (1)
#define MICROPY_PY_BUILTINS_FROZENSET (1)
#define MICROPY_PY_BUILTINS_SLICE_ATTRS (1)
#define MICROPY_PY_BUILTINS_ROUND_INT (1)
#define MICROPY_PY_ALL_SPECIAL_METHODS (1)
#define MICROPY_PY_BUILTINS_COMPILE (1)
#define MICROPY_PY_BUILTINS_EXECFILE (1)
#define MICROPY_PY_BUILTINS_NOTIMPLEMENTED (1)
#define MICROPY_PY_BUILTINS_INPUT   (1)
#define MICROPY_PY_BUILTINS_POW3    (0)
#define MICROPY_PY_BUILTINS_HELP    (1)
#define MICROPY_PY_BUILTINS_HELP_TEXT mt7697hdk_help_text
#define MICROPY_PY_BUILTINS_HELP_MODULES (1)
#define MICROPY_PY_MICROPYTHON_MEM_INFO (1)
#define MICROPY_PY_ARRAY_SLICE_ASSIGN (1)
#define MICROPY_PY_ARRAY              (1)
#define MICROPY_PY_COLLECTIONS       (1)
#define MICROPY_PY_COLLECTIONS_DEQUE (1)
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT (1)
#define MICROPY_PY_MATH_SPECIAL_FUNCTIONS (1)
#define MICROPY_PY_MATH_FACTORIAL   (1)
#define MICROPY_PY_CMATH            (1)
#define MICROPY_PY_IO               (1)
#define MICROPY_PY_IO_IOBASE        (1)
#define MICROPY_PY_IO_FILEIO        (MICROPY_VFS_FAT) // because mp_type_fileio/textio point to fatfs impl
#define MICROPY_PY_STRUCT           (1)
#define MICROPY_PY_SYS_MAXSIZE      (1)
#define MICROPY_PY_SYS_EXIT         (1)
#define MICROPY_PY_SYS_STDFILES     (1)
#define MICROPY_PY_SYS_STDIO_BUFFER (1)
#define MICROPY_PY_UERRNO           (1)
#ifndef MICROPY_PY_THREAD
#define MICROPY_PY_THREAD           (0)
#endif
#define MICROPY_MODULE_BUILTIN_INIT        (1)

// extended modules
#define MICROPY_PY_UCTYPES          (0)
#define MICROPY_PY_UZLIB            (1)
#define MICROPY_PY_UJSON            (1)
#define MICROPY_PY_URE              (1)
#define MICROPY_PY_URE_SUB          (1)
#define MICROPY_PY_UHEAPQ           (1)
#define MICROPY_PY_UHASHLIB         (1)
#define MICROPY_PY_UHASHLIB_SHA1    (1)
#define MICROPY_PY_UHASHLIB_SHA256          (1)
#define MICROPY_PY_UHASHLIB_MD5             (1)
#define MICROPY_PY_UCRYPTOLIB               (0)
#define MICROPY_PY_UBINASCII        (1)
#define MICROPY_PY_URANDOM          (0)
#define MICROPY_PY_URANDOM_EXTRA_FUNCS (0)
#define MICROPY_PY_USELECT          (1)
#define MICROPY_PY_UTIMEQ           (1)
#define MICROPY_PY_UTIME_MP_HAL     (1)
#define MICROPY_PY_OS_DUPTERM       (1)
#define MICROPY_PY_MACHINE          (1)
#define MICROPY_PY_MACHINE_PULSE    (1)
#define MICROPY_PY_MACHINE_PIN_MAKE_NEW mp_pin_make_new
#define MICROPY_PY_MACHINE_I2C      (1)
#if MICROPY_PY_MACHINE_I2C
#define MICROPY_PY_MACHINE_HW_I2C   (1)
#if MICROPY_PY_MACHINE_HW_I2C
#define MICROPY_PY_MACHINE_I2C_MAKE_NEW machine_hard_i2c_make_new
#endif
#endif
#define MICROPY_PY_MACHINE_SPI      (1)
#if MICROPY_PY_MACHINE_SPI
#define MICROPY_PY_MACHINE_HW_SPI   (1)
#if MICROPY_PY_MACHINE_HW_SPI
#define MICROPY_PY_MACHINE_SPI_MSB  (HAL_SPI_MASTER_MSB_FIRST)
#define MICROPY_PY_MACHINE_SPI_LSB  (HAL_SPI_MASTER_LSB_FIRST)
#define MICROPY_PY_MACHINE_SPI_MAKE_NEW machine_hw_spi_make_new
#endif
#endif
// #define MICROPY_HW_SOFTSPI_MIN_DELAY (0)
// #define MICROPY_HW_SOFTSPI_MAX_BAUDRATE (HAL_RCC_GetSysClockFreq() / 48)
#define MICROPY_PY_USSL                     (1)
#define MICROPY_SSL_AXTLS                   (0)
#define MICROPY_SSL_MBEDTLS                 (1)
#define MICROPY_PY_USSL_FINALISER           (1)
#define MICROPY_PY_FRAMEBUF         (0)
#ifndef MICROPY_PY_USOCKET
#define MICROPY_PY_USOCKET          (0)
#endif
#ifndef MICROPY_PY_NETWORK
#define MICROPY_PY_NETWORK          (0)
#endif

#define MICROPY_PY_UWEBSOCKET       (1)
#define MICROPY_PY_WEBREPL          (1)
#define MICROPY_PY_USOCKET_EVENTS    (MICROPY_PY_WEBREPL)
// TODO these should be generic, not bound to fatfs
#define mp_type_fileio mp_type_vfs_fat_fileio
#define mp_type_textio mp_type_vfs_fat_textio

// fatfs configuration used in ffconf.h
#define MICROPY_FATFS_ENABLE_LFN       (1)
#define MICROPY_FATFS_LFN_CODE_PAGE    437 /* 1=SFN/ANSI 437=LFN/U.S.(OEM) */
#define MICROPY_FATFS_USE_LABEL        (1)
#define MICROPY_FATFS_RPATH            (2)
#define MICROPY_FATFS_MULTI_PARTITION  (1)
#define MICROPY_FATFS_MAX_SS                (4096)

// use vfs's functions for import stat and builtin open
#define mp_import_stat mp_vfs_import_stat
#define mp_builtin_open mp_vfs_open
#define mp_builtin_open_obj mp_vfs_open_obj

// type definitions for the specific machine

#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void*)((mp_uint_t)(p) | 1))

// This port is intended to be 32-bit, but unfortunately, int32_t for
// different targets may be defined in different ways - either as int
// or as long. This requires different printf formatting specifiers
// to print such value. So, we avoid int32_t and use int directly.

// type definitions for the specific machine
#define MP_SSIZE_MAX (0x7fffffff)
#define UINT_FMT "%u"
#define INT_FMT "%d"
typedef int mp_int_t; // must be pointer size
typedef unsigned mp_uint_t; // must be pointer size

typedef long mp_off_t;

// extra built in modules to add to the list of known ones
extern const struct _mp_obj_module_t mp_module_machine;
extern const struct _mp_obj_module_t mp_module_uos;
extern const struct _mp_obj_module_t mp_module_utime;
extern const struct _mp_obj_module_t machine_module_network;
extern const struct _mp_obj_module_t mp_module_usocket;
extern const struct _mp_obj_module_t mp_module_uwebsocket;
extern const struct _mp_obj_module_t mp_module_webrepl;

#if MICROPY_PY_USOCKET_EVENTS
#define MICROPY_PY_USOCKET_EVENTS_HANDLER extern void usocket_events_handler(void);usocket_events_handler();
#else
#define MICROPY_PY_USOCKET_EVENTS_HANDLER
#endif

#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn_cooked(str, len)

#include "FreeRTOS.h"

#if MICROPY_PY_THREAD
#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(void); \
        mp_handle_pending(); \
        MICROPY_PY_USOCKET_EVENTS_HANDLER; \
        MP_THREAD_GIL_EXIT(); \
        MP_THREAD_GIL_ENTER(); \
    } while (0);
#else
#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(void); \
        mp_handle_pending(); \
        MICROPY_PY_USOCKET_EVENTS_HANDLER; \
        __WFI(); \
    } while (0);
#endif

// extra built in names to add to the global namespace
#define MICROPY_PORT_BUILTINS \
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&mp_builtin_open_obj) }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_uwebsocket), (mp_obj_t)&mp_module_uwebsocket }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR__webrepl), (mp_obj_t)&mp_module_webrepl }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_input), (mp_obj_t)&mp_builtin_input_obj }, \



// We need to provide a declaration/definition of alloca()
#include <alloca.h>

// board specifics
#define MICROPY_HW_BOARD_NAME "MT7697 module"
#define MICROPY_HW_MCU_NAME "MT7697"
#define MICROPY_PY_SYS_PLATFORM "mt7697"

#ifdef __linux__
#define MICROPY_MIN_USE_STDOUT (1)
#endif

#ifdef __thumb__
#define MICROPY_MIN_USE_CORTEX_CPU (1)
#define MICROPY_MIN_USE_STM32_MCU (0)
#endif

#define MP_STATE_PORT MP_STATE_VM


#define MICROPY_PORT_ROOT_POINTERS \
	mp_obj_t machine_irq_callback[18]; \
    const char *readline_hist[8]; \
    \
    /* pointers to all UART objects (if they have been created) */ \
    struct _machine_uart_obj_t *machine_uart_obj_all[MICROPY_HW_MAX_UART]; \


#define MICROPY_PORT_BUILTIN_MODULES \
    { MP_OBJ_NEW_QSTR(MP_QSTR_umachine), MP_ROM_PTR(&mp_module_machine) }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_uos), MP_ROM_PTR(&mp_module_uos) }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_utime), MP_ROM_PTR(&mp_module_utime) }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_network), MP_ROM_PTR(&machine_module_network) }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_usocket), MP_ROM_PTR(&mp_module_usocket) }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_uhashlib), (mp_obj_t)&mp_module_uhashlib }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_machine), MP_ROM_PTR(&mp_module_machine) }, \


#define MICROPY_PORT_BUILTIN_MODULE_WEAK_LINKS \
    { MP_OBJ_NEW_QSTR(MP_QSTR_ubinascii), (mp_obj_t)&mp_module_ubinascii }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_ucollections), MP_ROM_PTR(&mp_module_collections) }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_errno), (mp_obj_t)&mp_module_uerrno }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_uhashlib), (mp_obj_t)&mp_module_uhashlib }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_heapq), (mp_obj_t)&mp_module_uheapq }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_io), (mp_obj_t)&mp_module_io }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_json), (mp_obj_t)&mp_module_ujson }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_uos), MP_ROM_PTR(&mp_module_uos) }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_re), (mp_obj_t)&mp_module_ure }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_select), (mp_obj_t)&mp_module_uselect }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_ssl), (mp_obj_t)&mp_module_ussl }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_struct), (mp_obj_t)&mp_module_ustruct }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_time), MP_ROM_PTR(&mp_module_utime) }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_zlib), (mp_obj_t)&mp_module_uzlib }, \


// extra constants
#define MICROPY_PORT_CONSTANTS \
    { MP_ROM_QSTR(MP_QSTR_umachine), MP_ROM_PTR(&mp_module_machine) }, \
    { MP_ROM_QSTR(MP_QSTR_machine), MP_ROM_PTR(&mp_module_machine) }, \


