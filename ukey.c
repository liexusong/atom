/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Liexusong <liexusong@qq.com>                                 |
  +----------------------------------------------------------------------+
*/

/* $Id: header 297205 2010-03-30 21:09:07Z johannes $ */

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "main/SAPI.h"
#include "php_ukey.h"
#include "spinlock.h"
#include "shm.h"

int ncpu;

/* True global resources - no need for thread safety here */
static int pid = -1;
static int module_init = 0;
static int le_ukey;
static int datacenter_id;
static int worker_id = -1;
static __uint64_t twepoch;
static char *memaddr;
static struct shm shmctx;
static ukey_context_t *context;
static atomic_t *lock;

/* {{{ ukey_functions[]
 *
 * Every user visible function must have an entry in ukey_functions[].
 */
const zend_function_entry ukey_functions[] = {
    PHP_FE(ukey_next_id, NULL)
    PHP_FE(ukey_to_timestamp, NULL)
    PHP_FE(ukey_to_machine, NULL)
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ ukey_module_entry
 */
zend_module_entry ukey_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "ukey",
    ukey_functions,
    PHP_MINIT(ukey),
    PHP_MSHUTDOWN(ukey),
    PHP_RINIT(ukey),
    PHP_RSHUTDOWN(ukey),
    PHP_MINFO(ukey),
#if ZEND_MODULE_API_NO >= 20010901
    "0.3", /* Replace with version number for your extension */
#endif
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_UKEY
ZEND_GET_MODULE(ukey)
#endif

ZEND_INI_MH(ukey_ini_datacenter_id)
{
    if (new_value_length == 0) {
        return FAILURE;
    }

    datacenter_id = atoi(new_value);
    if (datacenter_id < 0) {
        return FAILURE;
    }

    return SUCCESS;
}

ZEND_INI_MH(ukey_ini_twepoch)
{
    if (new_value_length == 0) {
        return FAILURE;
    }

    sscanf(new_value, "%llu", &twepoch);
    if (twepoch <= 0ULL) {
        return FAILURE;
    }

    return SUCCESS;
}

PHP_INI_BEGIN()
    PHP_INI_ENTRY("ukey.datacenter", "0", PHP_INI_ALL,
          ukey_ini_datacenter_id)
    PHP_INI_ENTRY("ukey.twepoch", "1288834974657", PHP_INI_ALL,
          ukey_ini_twepoch)
PHP_INI_END()


int
ukey_startup(__uint64_t twepoch, int datacenter_id)
{
    /* If is CLI's SAPI,
     * we don't use share memory. */
    if (!strcasecmp(sapi_module.name, "cli")) {

        memaddr = malloc(sizeof(atomic_t) + sizeof(ukey_context_t));
        if (!memaddr) {
            php_printf("Fatal error: Not enough memory.\n");
            return -1;
        }

        lock = memaddr;
        context = memaddr + sizeof(atomic_t);

    } else {
        shmctx.size = sizeof(atomic_t) + sizeof(ukey_context_t);
        if (shm_alloc(&shmctx) == -1) {
            php_printf("Fatal error: Not enough memory.\n");
            return -1;
        }

        lock = shmctx.addr;
        context = (char *)shmctx.addr + sizeof(atomic_t);
    }

    *lock = 0; /* set lock to zero */

    context->twepoch = twepoch;
    context->datacenter_id = datacenter_id;

    context->sequence = 0;
    context->last_timestamp = 0ULL;

    context->worker_id_bits = 5;
    context->datacenter_id_bits = 5;
    context->sequence_bits = 12;

    context->worker_id_shift = context->sequence_bits;
    context->datacenter_id_shift = context->sequence_bits +
          context->worker_id_bits;
    context->timestamp_left_shift = context->sequence_bits +
          context->worker_id_bits + context->datacenter_id_bits;
    context->sequence_mask = -1 ^ (-1 << context->sequence_bits);

    return 0;
}


void
ukey_shutdown()
{
    if (!module_init) {
        return;
    }

    if (!strcasecmp(sapi_module.name, "cli")) {
        free(memaddr);
    } else {
        shm_free(&shmctx);
    }

    module_init = 0;
}


static void
exit_cb(void)
{
    if (lock == pid) {
        spin_unlock(lock, pid);
    }
    ukey_shutdown();
}


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(ukey)
{
    REGISTER_INI_ENTRIES();

    if (ukey_startup(twepoch, datacenter_id) == -1) {
        return FAILURE;
    }

    ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpu <= 0) {
        ncpu = 1;
    }

    atexit(exit_cb);

    module_init = 1;

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(ukey)
{
    UNREGISTER_INI_ENTRIES();

    ukey_shutdown();

    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(ukey)
{
    if (pid == -1) { /* init process ID */
        pid = (int)getpid();
    }

    if (worker_id == -1) { /* init worker ID */
        worker_id = pid & 0x1F;
    }

    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(ukey)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(ukey)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "ukey support", "enabled");
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}
/* }}} */


static __uint64_t
realtime()
{
    struct timeval tv;
    __uint64_t retval;

    if (gettimeofday(&tv, NULL) == -1) {
        return 0ULL;
    }

    retval = (__uint64_t)tv.tv_sec * 1000ULL + 
             (__uint64_t)tv.tv_usec / 1000ULL;

    return retval;
}


static __uint64_t
skip_next_millis()
{
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 1000; /* 1 millisecond */

    select(0, NULL, NULL, NULL, &tv); /* waiting */

    return realtime();
}


/* Every user-visible function in PHP should document itself in the source */

/* {{{ proto string ukey_next_id(void) */
PHP_FUNCTION(ukey_next_id)
{
    __uint64_t timestamp = realtime();
    __uint64_t retval;
    int len;
    char buf[128];

    if (timestamp == 0ULL) {
        RETURN_FALSE;
    }

    spin_lock(lock, pid);

    if (context->last_timestamp == timestamp) {
        context->sequence = (context->sequence + 1) & context->sequence_mask;
        if (context->sequence == 0) {
            timestamp = skip_next_millis();
        }

    } else {
        context->sequence = 0; /* Back to zero */
    }

    context->last_timestamp = timestamp;

    retval = ((timestamp - context->twepoch) << context->timestamp_left_shift)
           | (context->datacenter_id << context->datacenter_id_shift)
           | (worker_id << context->worker_id_shift)
           | context->sequence;

    spin_unlock(lock, pid);

    len = sprintf(buf, "%llu", retval);

    RETURN_STRINGL(buf, len, 1);
}
/* }}} */


static __uint64_t
ukey_change_uint64(char *key)
{
    __uint64_t retval;

    if (sscanf(key, "%llu", &retval) == 0) {
        return 0;
    }

    return retval;
}


/* {{{ proto int ukey_to_timestamp(string key) */
PHP_FUNCTION(ukey_to_timestamp)
{
    __uint64_t id;
    char *key;
    int len, ts;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, 
            &len TSRMLS_CC) == FAILURE)
    {
        RETURN_FALSE;
    }

    id = ukey_change_uint64(key);
    if (!id) {
        RETURN_FALSE;
    }

    /* Don't need lock share here,
     * Because timestamp_left_shift and twepoch unchanging */
    id = (id >> context->timestamp_left_shift) + context->twepoch;
    ts = id / 1000ULL;

    RETURN_LONG(ts);
}
/* }}} */


/* {{{ proto array ukey_to_machine(string key) */
PHP_FUNCTION(ukey_to_machine)
{
    __uint64_t id;
    int datacenter, worker;
    char *key;
    int len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, 
            &len TSRMLS_CC) == FAILURE)
    {
        RETURN_FALSE;
    }

    id = ukey_change_uint64(key);
    if (!id) {
        RETURN_FALSE;
    }

    /* Don't need lock share here,
     * Because datacenter_id_shift and worker_id_shift unchanging */
    datacenter = (id >> context->datacenter_id_shift) & 0x1FULL;
    worker = (id >> context->worker_id_shift) & 0x1FULL;

    array_init(return_value);
    add_assoc_long(return_value, "datacenter", datacenter);
    add_assoc_long(return_value, "worker", worker);

    return;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
