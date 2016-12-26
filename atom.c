/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
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

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_atom.h"
#include "spinlock.h"
#include "shm.h"

#ifdef __x86_64__
typedef unsigned long u64_t;
#else
typedef unsigned long long u64_t;
#endif

struct context {
    atomic_t lock;  /* Lock context */

    long sequence;
    u64_t last_ts;

    /* Various once initialized variables */
    int datacenter_id;
    int worker_id;
    u64_t twepoch;
    unsigned char worker_id_bits;
    unsigned char datacenter_id_bits;
    unsigned char sequence_bits;
    int worker_id_shift;
    int datacenter_id_shift;
    int timestamp_left_shift;
    int sequence_mask;
};

/* If you declare any globals in php_atom.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(atom)
*/

/* True global resources - no need for thread safety here */
static int le_atom;
static struct shm shmem;
static struct context *context;
static pid_t current_pid = -1;

static int datacenter_id;
static int worker_id;
static u64_t twepoch;

ZEND_INI_MH(atom_ini_set_datacenter_id)
{
#if ZEND_MODULE_API_NO >= 20151012  /* PHP7 */

    if (ZSTR_LEN(new_value) == 0) {
        return FAILURE;
    }

    datacenter_id = atoi(ZSTR_VAL(new_value));
    if (datacenter_id < 0 || datacenter_id > 31) {
        return FAILURE;
    }

    return SUCCESS;

#else

    if (new_value_length == 0) {
        return FAILURE;
    }

    datacenter_id = atoi(new_value);
    if (datacenter_id < 0 || datacenter_id > 31) {
        return FAILURE;
    }

    return SUCCESS;

#endif
}

ZEND_INI_MH(atom_ini_set_worker)
{
#if ZEND_MODULE_API_NO >= 20151012

    if (ZSTR_LEN(new_value) == 0) {
        return FAILURE;
    }

    worker_id = atoi(ZSTR_VAL(new_value));
    if (worker_id < 0 || worker_id > 31) {
        return FAILURE;
    }

    return SUCCESS;

#else

    if (new_value_length == 0) {
        return FAILURE;
    }

    worker_id = atoi(new_value);
    if (worker_id < 0 || worker_id > 31) {
        return FAILURE;
    }

    return SUCCESS;

#endif
}

ZEND_INI_MH(atom_ini_set_twepoch)
{
#if ZEND_MODULE_API_NO >= 20151012

    if (ZSTR_LEN(new_value) == 0) {
        return FAILURE;
    }

    sscanf(ZSTR_VAL(new_value), "%llu", &twepoch);
    if (twepoch <= 0ULL) {
        return FAILURE;
    }

    return SUCCESS;

#else

    if (new_value_length == 0) {
        return FAILURE;
    }

    sscanf(new_value, "%llu", &twepoch);
    if (twepoch <= 0ULL) {
        return FAILURE;
    }

    return SUCCESS;

#endif
}

PHP_INI_BEGIN()
    PHP_INI_ENTRY("atom.datacenter", "0", PHP_INI_ALL,
          atom_ini_set_datacenter_id)
    PHP_INI_ENTRY("atom.worker", "0", PHP_INI_ALL,
          atom_ini_set_datacenter_id)
    PHP_INI_ENTRY("atom.twepoch", "1451606400000", PHP_INI_ALL,
          atom_ini_set_twepoch)
PHP_INI_END()


static u64_t realtime()
{
    struct timeval tv;
    u64_t retval;

    if (gettimeofday(&tv, NULL) == -1) {
        return 0ULL;
    }

    retval = (u64_t)tv.tv_sec * 1000ULL +
             (u64_t)tv.tv_usec / 1000ULL;

    return retval;
}

static u64_t
skip_next_millis(u64_t last_ts)
{
    u64_t current;

    for (;;) {
        current = realtime();
        if (current > last_ts) {
            break;
        }
    }

    return current;
}


PHP_FUNCTION(atom_next_id)
{
    u64_t current = realtime();
    u64_t retval;
    int len;
    char buf[128];

    if (current == 0ULL) {
        RETURN_FALSE;
    }

    /* Make sure one process get the lock at the same time */
    spin_lock(&context->lock, (int)current_pid);

    if (context->last_ts == current) {
        context->sequence = (context->sequence + 1) & context->sequence_mask;
        if (context->sequence == 0) {
            current = skip_next_millis(context->last_ts);
        }

    } else {
        context->sequence = 0;
    }

    context->last_ts = current;

    retval = ((current - context->twepoch) << context->timestamp_left_shift)
            | (context->datacenter_id << context->datacenter_id_shift)
            | (context->worker_id << context->worker_id_shift)
            | context->sequence;

    spin_unlock(&context->lock, (int)current_pid);

    len = sprintf(buf, "%llu", retval);

    RETURN_STRINGL(buf, len, 1);
}


PHP_FUNCTION(atom_get_time)
{
    u64_t id;
    char *key;
    int len, ts;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key,
            &len TSRMLS_CC) == FAILURE)
    {
        RETURN_FALSE;
    }

    if (sscanf(key, "%llu", &id) == 0) {
        RETURN_FALSE;
    }

    /**
     * Don't need locked here,
     * because timestamp_left_shift and twepoch unchanging
     */

    id = (id >> context->timestamp_left_shift) + context->twepoch;
    ts = id / 1000ULL;

    RETURN_LONG(ts);
}


static int module_inited = 0;

int startup_atom_module()
{
    shmem.size = sizeof(struct context);
    if (shm_alloc(&shmem) == -1) {
        return -1;
    }

    context = (struct context *)shmem.addr;

    context->lock = 0;

    /* would changing */
    context->sequence = 0;
    context->last_ts = 0ULL;

    /* would not changing */
    context->datacenter_id = datacenter_id;
    context->worker_id = worker_id;
    context->twepoch = twepoch;

    context->worker_id_bits = 5;
    context->datacenter_id_bits = 5;
    context->sequence_bits = 12;

    context->worker_id_shift = context->sequence_bits;

    context->datacenter_id_shift = context->sequence_bits
                                 + context->worker_id_bits;

    context->timestamp_left_shift = context->sequence_bits
                                  + context->worker_id_bits
                                  + context->datacenter_id_bits;

    context->sequence_mask = -1 ^ (-1 << context->sequence_bits);

    module_inited = 1;

    spin_init();

    return 0;
}

void shutdown_atom_module()
{
    if (module_inited) {
        shm_free(&shmem);
        module_inited = 0;
    }
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(atom)
{
	REGISTER_INI_ENTRIES();

    if (startup_atom_module() == -1) {
        return FAILURE;
    }

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(atom)
{
	UNREGISTER_INI_ENTRIES();

    shutdown_atom_module();

	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(atom)
{
    if (current_pid == -1) {
        current_pid = getpid();
    }

	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(atom)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(atom)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "atom support", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ atom_functions[]
 *
 * Every user visible function must have an entry in atom_functions[].
 */
const zend_function_entry atom_functions[] = {
	PHP_FE(atom_next_id,     NULL)
    PHP_FE(atom_get_time,    NULL)
	PHP_FE_END	/* Must be the last line in atom_functions[] */
};
/* }}} */

/* {{{ atom_module_entry
 */
zend_module_entry atom_module_entry = {
	STANDARD_MODULE_HEADER,
	"atom",
	atom_functions,
	PHP_MINIT(atom),
	PHP_MSHUTDOWN(atom),
	PHP_RINIT(atom),
	PHP_RSHUTDOWN(atom),
	PHP_MINFO(atom),
	PHP_ATOM_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_ATOM
ZEND_GET_MODULE(atom)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
