/* bsdiff extension for PHP */

#ifndef PHP_BSDIFF_H
#define PHP_BSDIFF_H

extern zend_module_entry bsdiff_module_entry;
# define phpext_bsdiff_ptr &bsdiff_module_entry

#define PHP_BSDIFF_VERSION "0.1.0-dev"

# if defined(ZTS) && defined(COMPILE_DL_BSDIFF)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

#if PHP_VERSION_ID < 80000
#define RETURN_THROWS() RETURN_FALSE
#endif

#endif	/* PHP_BSDIFF_H */
