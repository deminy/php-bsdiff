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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"

#include "bsdiff.h"
#include "bspatch.h"
#include "php_bsdiff.h"
#include "php_bsdiff_arginfo.h"

#include <bzlib.h>

zend_class_entry *ce_bsdiff_exception;

void offtout(int64_t x,uint8_t *buf);
int64_t offtin(uint8_t *buf);

/* Wrappers around PHP's emalloc/efree for use as function pointers.
 * emalloc/efree are macros, so they cannot be assigned directly. */
static void *php_bsdiff_emalloc(size_t size)
{
    return emalloc(size);
}

static void php_bsdiff_efree(void *ptr)
{
    efree(ptr);
}

static int bz2_write(struct bsdiff_stream* stream, const void* buffer, int size)
{
    int bz2err;
    BZFILE* bz2;

    bz2 = (BZFILE*)stream->opaque;
    BZ2_bzWrite(&bz2err, bz2, (void*)buffer, size);
    if (bz2err != BZ_STREAM_END && bz2err != BZ_OK)
        return -1;

    return 0;
}

static int bz2_read(const struct bspatch_stream* stream, void* buffer, int length)
{
    int n;
    int bz2err;
    BZFILE* bz2;

    bz2 = (BZFILE*)stream->opaque;
    n = BZ2_bzRead(&bz2err, bz2, buffer, length);
    if (n != length)
        return -1;

    return 0;
}

/* {{{ void bsdiff_diff( string $old_file, string $new_file, string $patch_file ) */
PHP_FUNCTION(bsdiff_diff)
{
    char *old_file, *new_file, *diff_file;
    size_t old_file_len, new_file_len, diff_file_len;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_PATH(old_file, old_file_len)
        Z_PARAM_PATH(new_file, new_file_len)
        Z_PARAM_PATH(diff_file, diff_file_len)
    ZEND_PARSE_PARAMETERS_END();

    int bz2err;
    int64_t oldsize, newsize;
    uint8_t buf[8];
    FILE *pf = NULL;
    BZFILE *bz2 = NULL;
    struct bsdiff_stream stream;
    zend_string *old_str = NULL;
    zend_string *new_str = NULL;
    php_stream *php_s;

    stream.malloc = php_bsdiff_emalloc;
    stream.free = php_bsdiff_efree;
    stream.write = bz2_write;

    /* Read old file into a managed zend_string */
    php_s = php_stream_open_wrapper(old_file, "rb", 0, NULL);
    if (!php_s) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to open the old file \"%s\"", old_file);
        goto cleanup;
    }
    old_str = php_stream_copy_to_mem(php_s, PHP_STREAM_COPY_ALL, 0);
    php_stream_close(php_s);

    /* Read new file into a managed zend_string */
    php_s = php_stream_open_wrapper(new_file, "rb", 0, NULL);
    if (!php_s) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to open the new file \"%s\"", new_file);
        goto cleanup;
    }
    new_str = php_stream_copy_to_mem(php_s, PHP_STREAM_COPY_ALL, 0);
    php_stream_close(php_s);

    oldsize = old_str ? (int64_t)ZSTR_LEN(old_str) : 0;
    newsize = new_str ? (int64_t)ZSTR_LEN(new_str) : 0;

    /* Create the patch file.
     *
     * fopen() with a FILE* is kept here because the bzip2 library requires it.
     * The "b" flag is required for correctness on Windows when the extension
     * is loaded by a host that has not set the MSVC global `_fmode = _O_BINARY`.
     * The PHP CLI and CGI SAPIs both set it at startup, so in practice every
     * PHP process on Windows already opens streams in binary mode regardless
     * of this flag, but specifying it explicitly removes that dependency and
     * documents the intent. The flag is a no-op on POSIX systems. */
    if ((pf = fopen(diff_file, "wb")) == NULL) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Cannot open the diff file \"%s\" in write mode", diff_file);
        goto cleanup;
    }

    /* Write header (signature+newsize) */
    offtout(newsize, buf);
    if (fwrite("ENDSLEY/BSDIFF43", 16, 1, pf) != 1 ||
        fwrite(buf, sizeof(buf), 1, pf) != 1) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to write header to the diff file");
        goto cleanup;
    }

    if (NULL == (bz2 = BZ2_bzWriteOpen(&bz2err, pf, 9, 0, 0))) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to prepare to write data to the diff file (bz2err=%d)", bz2err);
        goto cleanup;
    }

    stream.opaque = bz2;
    if (bsdiff(
            old_str ? (const uint8_t *)ZSTR_VAL(old_str) : (const uint8_t *)"",
            oldsize,
            new_str ? (const uint8_t *)ZSTR_VAL(new_str) : (const uint8_t *)"",
            newsize,
            &stream)) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to create diff data");
        goto cleanup;
    }

    BZ2_bzWriteClose(&bz2err, bz2, 0, NULL, NULL);
    bz2 = NULL;
    if (bz2err != BZ_OK) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to complete writing data to the diff file (bz2err=%d)", bz2err);
        goto cleanup;
    }

    fclose(pf);
    pf = NULL;

cleanup:
    if (bz2) {
        BZ2_bzWriteClose(&bz2err, bz2, 0, NULL, NULL);
    }
    if (pf) {
        fclose(pf);
    }
    if (new_str) {
        zend_string_release(new_str);
    }
    if (old_str) {
        zend_string_release(old_str);
    }
}
/* }}} */

/* {{{ void bsdiff_patch( string $old_file, string $new_file, string $patch_file ) */
PHP_FUNCTION(bsdiff_patch)
{
    char *old_file, *new_file, *diff_file;
    size_t old_file_len, new_file_len, diff_file_len;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_PATH(old_file, old_file_len)
        Z_PARAM_PATH(new_file, new_file_len)
        Z_PARAM_PATH(diff_file, diff_file_len)
    ZEND_PARSE_PARAMETERS_END();

    int bz2err;
    uint8_t header[24];
    uint8_t *new_buf = NULL;
    int64_t oldsize, newsize;
    FILE *f = NULL;
    BZFILE *bz2 = NULL;
    struct bspatch_stream stream;
    zend_stat_t sb;
    zend_string *old_str = NULL;
    php_stream *php_s;

    /* Open patch file. fopen() with a FILE* is kept here because the bzip2
     * library requires it. See the matching comment in bsdiff_diff() for why
     * the "b" flag is specified explicitly. */
    if ((f = fopen(diff_file, "rb")) == NULL) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Cannot open diff file \"%s\" in read mode", diff_file);
        goto cleanup;
    }

    /* Read header */
    if (fread(header, 1, 24, f) != 24) {
        if (feof(f)) {
            zend_throw_exception_ex(ce_bsdiff_exception, 0, "The diff file is corrupted (missing header information)");
        } else {
            zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to read data from the diff file");
        }
        goto cleanup;
    }

    /* Check for appropriate magic */
    if (memcmp(header, "ENDSLEY/BSDIFF43", 16) != 0) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "The diff file is corrupted (invalid header information)");
        goto cleanup;
    }

    /* Read lengths from header */
    newsize = offtin(header + 16);
    if (newsize < 0) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "The diff file is corrupted (invalid length information)");
        goto cleanup;
    }

    /* Stat the old file to preserve its permissions on the output */
    if (VCWD_STAT(old_file, &sb) != 0) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to stat the old file \"%s\"", old_file);
        goto cleanup;
    }

    /* Read old file into a managed zend_string */
    php_s = php_stream_open_wrapper(old_file, "rb", 0, NULL);
    if (!php_s) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to open the old file \"%s\"", old_file);
        goto cleanup;
    }
    old_str = php_stream_copy_to_mem(php_s, PHP_STREAM_COPY_ALL, 0);
    php_stream_close(php_s);

    oldsize = old_str ? (int64_t)ZSTR_LEN(old_str) : 0;

    /* Allocate buffer for patched output; bspatch() writes into it */
    new_buf = emalloc(newsize + 1);

    if (NULL == (bz2 = BZ2_bzReadOpen(&bz2err, f, 0, 0, NULL, 0))) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to read data from the diff file (bz2err=%d)", bz2err);
        goto cleanup;
    }

    stream.read = bz2_read;
    stream.opaque = bz2;
    if (bspatch(
            old_str ? (const uint8_t *)ZSTR_VAL(old_str) : (const uint8_t *)"",
            oldsize,
            new_buf,
            newsize,
            &stream)) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to apply diff data");
        goto cleanup;
    }

    /* Clean up the bzip2 reads and diff file before writing output */
    BZ2_bzReadClose(&bz2err, bz2);
    bz2 = NULL;
    fclose(f);
    f = NULL;

    /* Write the new file */
    php_s = php_stream_open_wrapper(new_file, "wb", 0, NULL);
    if (!php_s) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to create the new file \"%s\"", new_file);
        goto cleanup;
    }
    if (newsize > 0 && (size_t)php_stream_write(php_s, (const char *)new_buf, (size_t)newsize) != (size_t)newsize) {
        php_stream_close(php_s);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to write to the new file \"%s\"", new_file);
        goto cleanup;
    }
    php_stream_close(php_s);

    /* Preserve original file permissions */
    VCWD_CHMOD(new_file, sb.st_mode);

cleanup:
    if (bz2) {
        BZ2_bzReadClose(&bz2err, bz2);
    }
    if (f) {
        fclose(f);
    }
    if (new_buf) {
        efree(new_buf);
    }
    if (old_str) {
        zend_string_release(old_str);
    }
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(bsdiff)
{
#if defined(ZTS) && defined(COMPILE_DL_BSDIFF)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(bsdiff)
{
    ce_bsdiff_exception = register_class_BsdiffException(zend_ce_exception);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(bsdiff)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "bsdiff support", "enabled");
    php_info_print_table_row(2, "bsdiff version", PHP_BSDIFF_VERSION);
    php_info_print_table_row(2, "BZip2 version", (char *) BZ2_bzlibVersion());
    php_info_print_table_end();
}
/* }}} */

/* {{{ bsdiff_module_entry */
zend_module_entry bsdiff_module_entry = {
    STANDARD_MODULE_HEADER,
    "bsdiff",           /* Extension name */
    ext_functions,      /* zend_function_entry */
    PHP_MINIT(bsdiff),  /* PHP_MINIT - Module initialization */
    NULL,               /* PHP_MSHUTDOWN - Module shutdown */
    PHP_RINIT(bsdiff),  /* PHP_RINIT - Request initialization */
    NULL,               /* PHP_RSHUTDOWN - Request shutdown */
    PHP_MINFO(bsdiff),  /* PHP_MINFO - Module info */
    PHP_BSDIFF_VERSION, /* Version */
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_BSDIFF
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(bsdiff)
#endif
