/*
   +----------------------------------------------------------------------+
   | bsdiff                                                               |
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | https://www.php.net/license/3_01.txt                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Demin Yin <deminy@deminy.net>                               |
   +----------------------------------------------------------------------+
*/

#ifndef PHP_BSDIFF_H
#define PHP_BSDIFF_H

extern zend_module_entry bsdiff_module_entry;
# define phpext_bsdiff_ptr &bsdiff_module_entry

#define PHP_BSDIFF_VERSION "0.1.3-dev"

# if defined(ZTS) && defined(COMPILE_DL_BSDIFF)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

#if PHP_VERSION_ID < 80000
#define RETURN_THROWS() RETURN_FALSE
#endif

#endif	/* PHP_BSDIFF_H */
