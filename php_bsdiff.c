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

#include <sys/stat.h>
#include <bzlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

zend_class_entry *ce_bsdiff_exception;

void offtout(int64_t x,uint8_t *buf);
int64_t offtin(uint8_t *buf);

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

    int fd;
    int bz2err;
    uint8_t *old,*new;
    off_t oldsize,newsize;
    uint8_t buf[8];
    FILE * pf;
    struct bsdiff_stream stream;
    BZFILE* bz2;

    memset(&bz2, 0, sizeof(bz2));
    stream.malloc = malloc;
    stream.free = free;
    stream.write = bz2_write;

    /* Allocate oldsize+1 bytes instead of oldsize bytes to ensure that we never try to malloc(0) and get a NULL pointer */
    if ((fd = open(old_file, O_RDONLY, 0)) < 0) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to open the old file \"%s\"", old_file);
        RETURN_THROWS();
    }
    if ((oldsize = lseek(fd, 0, SEEK_END)) == -1) {
        close(fd);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to determine size of the old file \"%s\"", old_file);
        RETURN_THROWS();
    }
    if ((old = malloc(oldsize + 1)) == NULL) {
        close(fd);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to allocate memory to store old data");
        RETURN_THROWS();
    }
    if ((lseek(fd, 0, SEEK_SET) != 0) || (read(fd, old, oldsize) != oldsize)) {
        free(old);
        close(fd);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to read data from the old file \"%s\"", old_file);
        RETURN_THROWS();
    }
    close(fd);

    /* Allocate newsize+1 bytes instead of newsize bytes to ensure that we never try to malloc(0) and get a NULL pointer */
    if ((fd = open(new_file, O_RDONLY, 0)) < 0) {
        free(old);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to open the new file \"%s\"", new_file);
        RETURN_THROWS();
    }
    if ((newsize = lseek(fd, 0, SEEK_END)) == -1) {
        free(old);
        close(fd);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to determine size of the new file \"%s\"", new_file);
        RETURN_THROWS();
    }
    if ((new = malloc(newsize + 1)) == NULL) {
        free(old);
        close(fd);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to allocate memory to store new data");
        RETURN_THROWS();
    }
    if ((lseek(fd, 0, SEEK_SET) != 0) || (read(fd, new, newsize) != newsize)) {
        free(old);
        free(new);
        close(fd);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to read data from the new file \"%s\"", new_file);
        RETURN_THROWS();
    }
    close(fd);

    /* Create the patch file */
    if ((pf = fopen(diff_file, "w")) == NULL) {
        free(old);
        free(new);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Cannot open the diff file \"%s\" in write mode", diff_file);
        RETURN_THROWS();
    }

    /* Write header (signature+newsize)*/
    offtout(newsize, buf);
    if (fwrite("ENDSLEY/BSDIFF43", 16, 1, pf) != 1 ||
        fwrite(buf, sizeof(buf), 1, pf) != 1) {
        free(old);
        free(new);
        fclose(pf);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to write header to the diff file");
        RETURN_THROWS();
    }


    if (NULL == (bz2 = BZ2_bzWriteOpen(&bz2err, pf, 9, 0, 0))) {
        free(old);
        free(new);
        fclose(pf);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to prepare to write data to the diff file (bz2err=%d)", bz2err);
        RETURN_THROWS();
    }

    stream.opaque = bz2;
    if (bsdiff(old, oldsize, new, newsize, &stream)) {
        free(old);
        free(new);
        BZ2_bzWriteClose(&bz2err, bz2, 0, NULL, NULL);
        fclose(pf);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to create diff data");
        RETURN_THROWS();
    }

    /* Free the memory we used */
    free(old);
    free(new);

    BZ2_bzWriteClose(&bz2err, bz2, 0, NULL, NULL);
    if (bz2err != BZ_OK) {
        fclose(pf);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to complete writing data to the diff file (bz2err=%d)", bz2err);
        RETURN_THROWS();
    }

    fclose(pf);
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

    FILE * f;
    int fd;
    int bz2err;
    uint8_t header[24];
    uint8_t *old, *new;
    int64_t oldsize, newsize;
    BZFILE* bz2;
    struct bspatch_stream stream;
    struct stat sb;

    /* Open patch file */
    if ((f = fopen(diff_file, "r")) == NULL) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Cannot open diff file \"%s\" in read mode", diff_file);
        RETURN_THROWS();
    }

    /* Read header */
    if (fread(header, 1, 24, f) != 24) {
        if (feof(f)) {
            fclose(f);
            zend_throw_exception_ex(ce_bsdiff_exception, 0, "The diff file is corrupted (missing header information)");
            RETURN_THROWS();
        }
        fclose(f);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to read data from the diff file");
        RETURN_THROWS();
    }

    /* Check for appropriate magic */
    if (memcmp(header, "ENDSLEY/BSDIFF43", 16) != 0) {
        fclose(f);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "The diff file is corrupted (invalid header information)");
        RETURN_THROWS();
    }

    /* Read lengths from header */
    newsize=offtin(header+16);
    if(newsize<0) {
        fclose(f);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "The diff file is corrupted (invalid length information)");
        RETURN_THROWS();
    }

    /* Close patch file and re-open it via libbzip2 at the right places */
    if ((fd = open(old_file, O_RDONLY, 0)) < 0) {
        fclose(f);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to open the old file \"%s\"", old_file);
        RETURN_THROWS();
    }
    if ((oldsize = lseek(fd, 0, SEEK_END)) == -1) {
        fclose(f);
        close(fd);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to determine size of the old file \"%s\"", old_file);
        RETURN_THROWS();
    }
    if ((old = malloc(oldsize + 1)) == NULL) {
        fclose(f);
        close(fd);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to allocate memory to store old data");
        RETURN_THROWS();
    }
    if ((lseek(fd, 0, SEEK_SET) != 0) || (read(fd, old, oldsize) != oldsize) || (fstat(fd, &sb))) {
        free(old);
        fclose(f);
        close(fd);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to read data from the old file \"%s\"", old_file);
        RETURN_THROWS();
    }
    close(fd);

    if((new=malloc(newsize+1))==NULL) {
        free(old);
        fclose(f);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to allocate memory to store patched data");
        RETURN_THROWS();
    }

    if (NULL == (bz2 = BZ2_bzReadOpen(&bz2err, f, 0, 0, NULL, 0))) {
        free(new);
        free(old);
        fclose(f);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to read data from the diff file (bz2err=%d)", bz2err);
        RETURN_THROWS();
    }

    stream.read = bz2_read;
    stream.opaque = bz2;
    if (bspatch(old, oldsize, new, newsize, &stream)) {
        free(new);
        free(old);
        BZ2_bzReadClose(&bz2err, bz2);
        fclose(f);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to apply diff data");
        RETURN_THROWS();
    }

    /* Clean up the bzip2 reads */
    BZ2_bzReadClose(&bz2err, bz2);
    fclose(f);

    /* Write the new file */
    if ((fd = open(new_file, O_CREAT | O_TRUNC | O_WRONLY, sb.st_mode)) < 0) {
        free(new);
        free(old);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to create the new file \"%s\"", new_file);
        RETURN_THROWS();
    }
    if (write(fd,new,newsize) != newsize) {
        free(new);
        free(old);
        close(fd);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to create the new file \"%s\"", new_file);
        RETURN_THROWS();
    }

    free(new);
    free(old);
    close(fd);
}
/* }}}*/

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
