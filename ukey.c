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
  | Author: YukChung Lie <liexusong@qq.com>                              |
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
#include "php_ukey.h"
#include "spinlock.h"
#include "shm.h"


typedef unsigned long long ukey_uint64;

typedef struct {
    int worker_id;
    int datacenter_id;

    long sequence;
    ukey_uint64 last_timestamp;

    /* Various once initialized variables */
    ukey_uint64 twepoch;
    unsigned char worker_id_bits;
    unsigned char datacenter_id_bits;
    unsigned char sequence_bits;
    int worker_id_shift;
    int datacenter_id_shift;
    int timestamp_left_shift;
    int sequence_mask;
} ukey_context_t;


int ncpu;

/* True global resources - no need for thread safety here */
static int le_ukey;
static int worker_id;
static int datacenter_id;
static ukey_uint64 twepoch;
static struct shm locker_shm;
static struct shm context_shm;
static ukey_context_t *context;
static int *lock;

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
    "0.1", /* Replace with version number for your extension */
#endif
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_UKEY
ZEND_GET_MODULE(ukey)
#endif


ZEND_INI_MH(ukey_ini_worker_id)
{
    if (new_value_length == 0) {
        return FAILURE;
    }

    worker_id = atoi(new_value);
    if (worker_id < 0) {
        return FAILURE;
    }

    return SUCCESS;
}

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
    PHP_INI_ENTRY("ukey.worker", "0", PHP_INI_ALL,
          ukey_ini_worker_id)
    PHP_INI_ENTRY("ukey.datacenter", "0", PHP_INI_ALL,
          ukey_ini_datacenter_id)
    PHP_INI_ENTRY("ukey.twepoch", "1288834974657", PHP_INI_ALL,
          ukey_ini_twepoch)
PHP_INI_END()


int ukey_startup(ukey_uint64 twepoch, int worker_id, int datacenter_id)
{
    locker_shm.size = sizeof(int);
    if (shm_alloc(&locker_shm) == -1) {
        return -1;
    }
    lock = locker_shm.addr; *lock = 0; /* init lock value */

    context_shm.size = sizeof(ukey_context_t);
    if (shm_alloc(&context_shm) == -1) {
        shm_free(&locker_shm);
        return -1;
    }
    context = context_shm.addr;

    context->twepoch = twepoch;
    context->worker_id = worker_id;
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


void ukey_shutdown()
{
    shm_free(&locker_shm);
    shm_free(&context_shm);
}


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(ukey)
{
    REGISTER_INI_ENTRIES();

    if (ukey_startup(twepoch, worker_id, datacenter_id) == -1) {
        return FAILURE;
    }

    ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpu <= 0) {
        ncpu = 1;
    }

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


static ukey_uint64 really_time()
{
    struct timeval tv;
    ukey_uint64 retval;

    if (gettimeofday(&tv, NULL) == -1) {
        return 0ULL;
    }

    retval = (ukey_uint64)tv.tv_sec * 1000ULL + 
             (ukey_uint64)tv.tv_usec / 1000ULL;

    return retval;
}


static ukey_uint64 skip_next_millis()
{
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 1000; /* one millisecond */

    select(0, NULL, NULL, NULL, &tv); /* wait here */

    return really_time();
}


/* Every user-visible function in PHP should document itself in the source */

/* {{{ proto string ukey_next_id(void) */
PHP_FUNCTION(ukey_next_id)
{
    ukey_uint64 timestamp = really_time();
    ukey_uint64 retval;
    int len;
    char sbuf[128];
    int pid = (int)getpid();

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

    retval = ((timestamp - context->twepoch) << context->timestamp_left_shift) |
          (context->datacenter_id << context->datacenter_id_shift) |
          (context->worker_id << context->worker_id_shift) |
          context->sequence;

    spin_unlock(lock, pid);

    len = sprintf(sbuf, "%llu", retval);

    RETURN_STRINGL(sbuf, len, 1);
}
/* }}} */


static ukey_uint64 ukey_change_uint64(char *key)
{
    ukey_uint64 retval;

    if (sscanf(key, "%llu", &retval) == 0) {
        return 0;
    }

    return retval;
}


/* {{{ proto int ukey_to_timestamp(string key) */
PHP_FUNCTION(ukey_to_timestamp)
{
    ukey_uint64 id;
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
    ukey_uint64 id;
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
