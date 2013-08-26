/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2010 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id: header 297205 2010-03-30 21:09:07Z johannes $ */

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_ukey.h"


#ifdef WIN32
typedef unsigned __int64 ukey_uint64;
#else
typedef unsigned long long ukey_uint64;
#endif

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


/* True global resources - no need for thread safety here */
static int le_ukey;
static int worker_id;
static int datacenter_id;
static ukey_context_t *_ctx;

/* {{{ ukey_functions[]
 *
 * Every user visible function must have an entry in ukey_functions[].
 */
const zend_function_entry ukey_functions[] = {
    PHP_FE(ukey_next_id, NULL)
    PHP_FE(ukey_to_timestamp, NULL)
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
    PHP_RINIT(ukey),        /* Replace with NULL if there's nothing to do at request start */
    PHP_RSHUTDOWN(ukey),    /* Replace with NULL if there's nothing to do at request end */
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

PHP_INI_BEGIN()
    PHP_INI_ENTRY("ukey.worker", "0", PHP_INI_ALL,
          ukey_ini_worker_id)
    PHP_INI_ENTRY("ukey.datacenter", "0", PHP_INI_ALL,
          ukey_ini_datacenter_id)
PHP_INI_END()


int ukey_startup(int worker_id, int datacenter_id)
{
    _ctx = malloc(sizeof(ukey_context_t));
    if (!_ctx) {
        return -1;
    }

    _ctx->worker_id = worker_id;
    _ctx->datacenter_id = datacenter_id;

    _ctx->sequence = 0;
    _ctx->last_timestamp = -1;

    _ctx->twepoch = 1288834974657LL;
    _ctx->worker_id_bits = 5;
    _ctx->datacenter_id_bits = 5;
    _ctx->sequence_bits = 12;

    _ctx->worker_id_shift = _ctx->sequence_bits;
    _ctx->datacenter_id_shift = _ctx->sequence_bits + _ctx->worker_id_bits;
    _ctx->timestamp_left_shift = _ctx->sequence_bits + _ctx->worker_id_bits + 
          _ctx->datacenter_id_bits;
    _ctx->sequence_mask = -1 ^ (-1 << _ctx->sequence_bits);

    return 0;
}


void ukey_shutdown()
{
    free(_ctx);
}


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(ukey)
{
    REGISTER_INI_ENTRIES();

    ukey_startup(worker_id, datacenter_id);

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

    /* Remove comments if you have entries in php.ini
    DISPLAY_INI_ENTRIES();
    */
}
/* }}} */


/* win32 helper function */

#if 0
static int
gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);

    tm.tm_year     = wtm.wYear - 1900;
    tm.tm_mon      = wtm.wMonth - 1;
    tm.tm_mday     = wtm.wDay;
    tm.tm_hour     = wtm.wHour;
    tm.tm_min      = wtm.wMinute;
    tm.tm_sec      = wtm.wSecond;
    tm. tm_isdst   = -1;

    clock = mktime(&tm);

    tp->tv_sec = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;

    return (0);
}
#endif


static ukey_uint64 realy_time()
{
    time_t ts;
    ukey_uint64 retval;

    time(&ts);

    retval = (ukey_uint64)ts * 1000;

    return retval;
}


static ukey_uint64 skip_next_millis()
{
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 1000; /* one millisecond */

    select(0, NULL, NULL, NULL, &tv);

    return realy_time();
}


/* Remove the following function when you have succesfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string ukey_next_id(void) */
PHP_FUNCTION(ukey_next_id)
{
    ukey_uint64 timestamp = realy_time();
    ukey_uint64 retval;
    int len;
    char sbuf[128];

    if (_ctx->last_timestamp == timestamp) {
        _ctx->sequence = (_ctx->sequence + 1) & _ctx->sequence_mask;

        if (_ctx->sequence == 0) {
            timestamp = skip_next_millis();
        }

    } else {
        _ctx->sequence = 0;
    }

    _ctx->last_timestamp = timestamp;

    retval = ((timestamp - _ctx->twepoch) << _ctx->timestamp_left_shift) |
          (_ctx->datacenter_id << _ctx->datacenter_id_shift) |
          (_ctx->worker_id << _ctx->worker_id_shift) |
          _ctx->sequence;

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

    id = (id >> _ctx->timestamp_left_shift) + _ctx->twepoch;
    ts = id / 1000;

    RETURN_LONG(ts);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
