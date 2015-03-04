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

#ifndef PHP_UKEY_H
#define PHP_UKEY_H

extern zend_module_entry ukey_module_entry;
#define phpext_ukey_ptr &ukey_module_entry

#ifdef PHP_WIN32
#	define PHP_UKEY_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_UKEY_API __attribute__ ((visibility("default")))
#else
#	define PHP_UKEY_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(ukey);
PHP_MSHUTDOWN_FUNCTION(ukey);
PHP_RINIT_FUNCTION(ukey);
PHP_RSHUTDOWN_FUNCTION(ukey);
PHP_MINFO_FUNCTION(ukey);

PHP_FUNCTION(ukey_next_id);
PHP_FUNCTION(ukey_to_timestamp);
PHP_FUNCTION(ukey_to_machine);

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(ukey)
	long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(ukey)
*/

/* In every utility function you add that needs to use variables 
   in php_ukey_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as UKEY_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define UKEY_G(v) TSRMG(ukey_globals_id, zend_ukey_globals *, v)
#else
#define UKEY_G(v) (ukey_globals.v)
#endif

typedef unsigned long long __uint64_t;

typedef struct {
    int datacenter_id;

    long sequence;
    __uint64_t last_timestamp;

    /* Various once initialized variables */
    __uint64_t twepoch;
    unsigned char worker_id_bits;
    unsigned char datacenter_id_bits;
    unsigned char sequence_bits;
    int worker_id_shift;
    int datacenter_id_shift;
    int timestamp_left_shift;
    int sequence_mask;
} ukey_context_t;

#endif	/* PHP_UKEY_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
