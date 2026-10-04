/*
   +----------------------------------------------------------------------+
   | bsdiff                                                               |
   +----------------------------------------------------------------------+
   | Copyright © Demin Yin and Contributors.                              |
   +----------------------------------------------------------------------+
   | This source file is subject to the Modified BSD License that is      |
   | bundled with this package in the file LICENSE.                       |
   |                                                                      |
   | SPDX-License-Identifier: BSD-3-Clause                                |
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

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif

#define PHP_BSDIFF_MAGIC     "ENDSLEY/BSDIFF43"
#define PHP_BSDIFF_BUF_SIZE  8192

static zend_class_entry *ce_bsdiff_exception;

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

/* Allocators for libbz2. Routing them through emalloc/efree makes the compressor/decompressor state count against
 * memory_limit, and lets PHP reclaim it if a fatal error (longjmp) skips our cleanup code. */
static void *php_bsdiff_bzalloc(void *opaque, int n, int m)
{
    return safe_emalloc((size_t)n, (size_t)m, 0);
}

static void php_bsdiff_bzfree(void *opaque, void *ptr)
{
    if (ptr) {
        efree(ptr);
    }
}

/* Remove a (partially written) output file through its stream wrapper, so that open_basedir is respected. */
static void php_bsdiff_unlink(const char *path)
{
    const char *path_for_open = path;
    php_stream_wrapper *wrapper = php_stream_locate_url_wrapper(path, &path_for_open, 0);

    if (wrapper && wrapper->wops->unlink) {
        wrapper->wops->unlink(wrapper, path, 0, NULL);
    }
}

/* Change the permissions of a file through its stream wrapper (the same way PHP's chmod() does for wrappers). */
static void php_bsdiff_chmod(const char *path, zend_long mode)
{
    const char *path_for_open = path;
    php_stream_wrapper *wrapper = php_stream_locate_url_wrapper(path, &path_for_open, 0);

    if (wrapper && wrapper->wops->stream_metadata) {
        wrapper->wops->stream_metadata(wrapper, path, PHP_STREAM_META_ACCESS, &mode, NULL);
    }
}

/* Read a whole input file into memory. Throws and returns NULL on failure.
 *
 * If mode is not NULL, it receives the permission bits of the file (or -1 if the file is not a regular file, or its
 * permissions are unknown). */
static zend_string *php_bsdiff_read_file(const char *path, const char *label, zend_long *mode)
{
    php_stream *s;
    php_stream_statbuf ssb;
    zend_string *str;
    int is_reg;

    s = php_stream_open_wrapper((char *)path, "rb", 0, NULL);
    if (!s) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to open the %s file \"%s\"", label, path);
        return NULL;
    }

    is_reg = 0;
    if (php_stream_stat(s, &ssb) == 0) {
        if (S_ISDIR(ssb.sb.st_mode)) {
            php_stream_close(s);
            zend_throw_exception_ex(ce_bsdiff_exception, 0, "The %s file \"%s\" is a directory", label, path);
            return NULL;
        }
        is_reg = S_ISREG(ssb.sb.st_mode);
    }

    str = php_stream_copy_to_mem(s, PHP_STREAM_COPY_ALL, 0);
    php_stream_close(s);
    if (!str) {
        str = ZSTR_EMPTY_ALLOC();
    }

    /* php_stream_copy_to_mem() reports read errors only as notices (or not at all on older PHP versions), and
     * returns whatever was read so far. For regular files, a short read means the read failed. A user error handler
     * may also have turned the notice into an exception. */
    if (EG(exception) || (is_reg && (uint64_t)ZSTR_LEN(str) < (uint64_t)ssb.sb.st_size)) {
        zend_string_release(str);
        if (!EG(exception)) {
            zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to read the %s file \"%s\"", label, path);
        }
        return NULL;
    }

    if (mode) {
        *mode = is_reg ? (zend_long)(ssb.sb.st_mode & 0777) : -1;
    }

    return str;
}

/* {{{ bzip2 compression of the diff data, written to a PHP stream */
typedef struct {
    bz_stream bz;
    php_stream *out;
    char buf[PHP_BSDIFF_BUF_SIZE];
} php_bsdiff_writer;

/* Run the compressor with the given action (BZ_RUN or BZ_FINISH) and write its output to the stream. */
static int php_bsdiff_writer_compress(php_bsdiff_writer *w, int action)
{
    int ret;
    size_t n;

    do {
        w->bz.next_out = w->buf;
        w->bz.avail_out = sizeof(w->buf);
        ret = BZ2_bzCompress(&w->bz, action);
        if (action == BZ_RUN ? ret != BZ_RUN_OK : (ret != BZ_FINISH_OK && ret != BZ_STREAM_END)) {
            return -1;
        }
        n = sizeof(w->buf) - w->bz.avail_out;
        if (n > 0 && (size_t)php_stream_write(w->out, w->buf, n) != n) {
            return -1;
        }
    } while (action == BZ_RUN ? w->bz.avail_in > 0 : ret != BZ_STREAM_END);

    return 0;
}

static int bz2_write(struct bsdiff_stream* stream, const void* buffer, int size)
{
    php_bsdiff_writer *w = (php_bsdiff_writer *)stream->opaque;

    if (size <= 0) {
        return size == 0 ? 0 : -1;
    }

    w->bz.next_in = (char *)buffer;
    w->bz.avail_in = (unsigned int)size;

    return php_bsdiff_writer_compress(w, BZ_RUN);
}

/* Write a complete diff (header + compressed data) to the given stream. Throws and returns FAILURE on failure. */
static int php_bsdiff_write_diff(php_stream *out, zend_string *old_str, zend_string *new_str)
{
    php_bsdiff_writer w;
    struct bsdiff_stream stream;
    uint8_t buf[8];
    int bz2err;
    int ret = FAILURE;

    /* Write header (signature+newsize) */
    offtout((int64_t)ZSTR_LEN(new_str), buf);
    if ((size_t)php_stream_write(out, PHP_BSDIFF_MAGIC, 16) != 16 ||
        (size_t)php_stream_write(out, (const char *)buf, sizeof(buf)) != sizeof(buf)) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to write header to the diff file");
        return FAILURE;
    }

    memset(&w.bz, 0, sizeof(w.bz));
    w.bz.bzalloc = php_bsdiff_bzalloc;
    w.bz.bzfree = php_bsdiff_bzfree;
    w.out = out;
    if ((bz2err = BZ2_bzCompressInit(&w.bz, 9, 0, 0)) != BZ_OK) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to prepare to write data to the diff file (bz2err=%d)", bz2err);
        return FAILURE;
    }

    stream.opaque = &w;
    stream.malloc = php_bsdiff_emalloc;
    stream.free = php_bsdiff_efree;
    stream.write = bz2_write;
    if (bsdiff(
            (const uint8_t *)ZSTR_VAL(old_str),
            (int64_t)ZSTR_LEN(old_str),
            (const uint8_t *)ZSTR_VAL(new_str),
            (int64_t)ZSTR_LEN(new_str),
            &stream)) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to create diff data");
    } else if (php_bsdiff_writer_compress(&w, BZ_FINISH)) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to complete writing data to the diff file");
    } else {
        ret = SUCCESS;
    }

    BZ2_bzCompressEnd(&w.bz);

    return ret;
}
/* }}} */

/* {{{ bzip2 decompression of the diff data, read from a PHP stream */
typedef enum {
    PHP_BSDIFF_READ_OK = 0,
    PHP_BSDIFF_READ_ERROR,      /* I/O error on the diff file */
    PHP_BSDIFF_READ_CORRUPTED,  /* invalid bzip2 data */
    PHP_BSDIFF_READ_TRUNCATED,  /* the diff file ends in the middle of the bzip2 stream */
    PHP_BSDIFF_READ_END,        /* the bzip2 stream ended before the requested data could be read */
    PHP_BSDIFF_READ_OVERSIZED,  /* more data requested than a valid diff can contain */
} php_bsdiff_read_status;

typedef struct {
    bz_stream bz;
    php_stream *in;
    int eof;                    /* no more input from the diff file */
    int stream_end;             /* end of the bzip2 stream reached */
    uint64_t total;             /* bytes handed to bspatch() so far */
    uint64_t budget;            /* upper limit of bytes bspatch() may request from a valid diff */
    php_bsdiff_read_status status;
    char buf[PHP_BSDIFF_BUF_SIZE];
} php_bsdiff_reader;

/* Decompress exactly length bytes into buffer. Returns 0 on success, or -1 with r->status set. */
static int php_bsdiff_reader_read(php_bsdiff_reader *r, char *buffer, unsigned int length)
{
    unsigned int avail_in, avail_out;
    ssize_t n;
    int ret;

    r->bz.next_out = buffer;
    r->bz.avail_out = length;
    while (r->bz.avail_out > 0) {
        if (r->stream_end) {
            r->status = PHP_BSDIFF_READ_END;
            return -1;
        }

        if (r->bz.avail_in == 0 && !r->eof) {
            n = (ssize_t)php_stream_read(r->in, r->buf, sizeof(r->buf));
            if (n < 0) {
                r->status = PHP_BSDIFF_READ_ERROR;
                return -1;
            }
            if (n == 0) {
                r->eof = 1;
            } else {
                r->bz.next_in = r->buf;
                r->bz.avail_in = (unsigned int)n;
            }
        }

        avail_in = r->bz.avail_in;
        avail_out = r->bz.avail_out;
        ret = BZ2_bzDecompress(&r->bz);
        if (ret == BZ_STREAM_END) {
            r->stream_end = 1;
        } else if (ret != BZ_OK) {
            r->status = PHP_BSDIFF_READ_CORRUPTED;
            return -1;
        } else if (r->bz.avail_in == avail_in && r->bz.avail_out == avail_out) {
            /* No progress: either the input is exhausted, or the decompressor is stuck. */
            r->status = r->eof ? PHP_BSDIFF_READ_TRUNCATED : PHP_BSDIFF_READ_CORRUPTED;
            return -1;
        }
    }

    return 0;
}

static int bz2_read(const struct bspatch_stream* stream, void* buffer, int length)
{
    php_bsdiff_reader *r = (php_bsdiff_reader *)stream->opaque;

    if (length < 0) {
        r->status = PHP_BSDIFF_READ_CORRUPTED;
        return -1;
    }

    /* A crafted diff can make bspatch() loop on control entries that produce no output, while a tiny, highly
     * compressible diff file feeds it an almost endless bzip2 stream. Since every control entry of a valid diff
     * produces at least one byte of output, a valid diff never needs more than this budget. */
    r->total += (uint64_t)length;
    if (r->total > r->budget) {
        r->status = PHP_BSDIFF_READ_OVERSIZED;
        return -1;
    }

    return php_bsdiff_reader_read(r, (char *)buffer, (unsigned int)length);
}

/* Make sure the diff data ends exactly where bspatch() stopped reading: the bzip2 stream must end (which also verifies
 * its CRC), and nothing may follow it. Returns 0 on success, or -1 with r->status set. */
static int php_bsdiff_reader_finish(php_bsdiff_reader *r)
{
    char c;

    if (php_bsdiff_reader_read(r, &c, 1) == 0) {
        r->status = PHP_BSDIFF_READ_OVERSIZED;
        return -1;
    }
    if (r->status != PHP_BSDIFF_READ_END) {
        return -1;
    }
    r->status = PHP_BSDIFF_READ_OK;

    if (r->bz.avail_in > 0 || (!r->eof && php_stream_read(r->in, &c, 1) > 0)) {
        r->status = PHP_BSDIFF_READ_OVERSIZED;
        return -1;
    }

    return 0;
}

static const char *php_bsdiff_read_error(php_bsdiff_read_status status)
{
    switch (status) {
        case PHP_BSDIFF_READ_ERROR:
            return "failed to read data";
        case PHP_BSDIFF_READ_CORRUPTED:
            return "invalid compressed data";
        case PHP_BSDIFF_READ_TRUNCATED:
        case PHP_BSDIFF_READ_END:
            return "unexpected end of data";
        case PHP_BSDIFF_READ_OVERSIZED:
            return "unexpected extra data";
        default:
            return "invalid control data";
    }
}
/* }}} */

/* {{{ void bsdiff_diff( string $old_file, string $new_file, string $diff_file ) */
PHP_FUNCTION(bsdiff_diff)
{
    char *old_file, *new_file, *diff_file;
    size_t old_file_len, new_file_len, diff_file_len;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_PATH(old_file, old_file_len)
        Z_PARAM_PATH(new_file, new_file_len)
        Z_PARAM_PATH(diff_file, diff_file_len)
    ZEND_PARSE_PARAMETERS_END();

    zend_string *old_str = NULL;
    zend_string *new_str = NULL;
    php_stream *diff_s = NULL;
    int ret = FAILURE;

    if (NULL == (old_str = php_bsdiff_read_file(old_file, "old", NULL))) {
        goto cleanup;
    }
    if (NULL == (new_str = php_bsdiff_read_file(new_file, "new", NULL))) {
        goto cleanup;
    }

    /* bsdiff() allocates (oldsize + 1) * sizeof(int64_t) bytes, which can overflow size_t on 32-bit platforms. */
    if ((uint64_t)ZSTR_LEN(old_str) >= (uint64_t)(SIZE_MAX / sizeof(int64_t) - 1)) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "The old file \"%s\" is too large", old_file);
        goto cleanup;
    }

    /* The "b" flag is required for correctness on Windows when the extension is loaded by a host that has not set the
     * MSVC global `_fmode = _O_BINARY`. The flag is a no-op on POSIX systems. */
    diff_s = php_stream_open_wrapper(diff_file, "wb", 0, NULL);
    if (!diff_s) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Cannot open the diff file \"%s\" in write mode", diff_file);
        goto cleanup;
    }

    /* If a fatal error (e.g., memory_limit exhausted) happens while creating the diff, remove the partial diff file
     * before passing the error on. Memory allocated by bsdiff and bzip2 comes from emalloc, so PHP reclaims it. */
    zend_try {
        ret = php_bsdiff_write_diff(diff_s, old_str, new_str);
    } zend_catch {
        php_stream_close(diff_s);
        php_bsdiff_unlink(diff_file);
        zend_bailout();
    } zend_end_try();

    php_stream_close(diff_s);
    if (ret != SUCCESS) {
        php_bsdiff_unlink(diff_file);
    }

cleanup:
    if (new_str) {
        zend_string_release(new_str);
    }
    if (old_str) {
        zend_string_release(old_str);
    }
    if (EG(exception)) {
        RETURN_THROWS();
    }
}
/* }}} */

/* {{{ void bsdiff_patch( string $old_file, string $new_file, string $diff_file ) */
PHP_FUNCTION(bsdiff_patch)
{
    char *old_file, *new_file, *diff_file;
    size_t old_file_len, new_file_len, diff_file_len;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_PATH(old_file, old_file_len)
        Z_PARAM_PATH(new_file, new_file_len)
        Z_PARAM_PATH(diff_file, diff_file_len)
    ZEND_PARSE_PARAMETERS_END();

    char header[24];
    size_t header_len;
    ssize_t n;
    int bz2err;
    uint8_t *new_buf = NULL;
    int64_t newsize;
    zend_long old_mode;
    int created;
    struct bspatch_stream stream;
    php_bsdiff_reader reader;
    int reader_initialized = 0;
    zend_string *old_str = NULL;
    php_stream *diff_s = NULL;
    php_stream *new_s = NULL;

    /* Open patch file. */
    diff_s = php_stream_open_wrapper(diff_file, "rb", 0, NULL);
    if (!diff_s) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Cannot open diff file \"%s\" in read mode", diff_file);
        goto cleanup;
    }

    /* Read header */
    header_len = 0;
    while (header_len < sizeof(header)) {
        n = (ssize_t)php_stream_read(diff_s, header + header_len, sizeof(header) - header_len);
        if (n <= 0) {
            break;
        }
        header_len += (size_t)n;
    }
    if (header_len != sizeof(header)) {
        if (php_stream_eof(diff_s)) {
            zend_throw_exception_ex(ce_bsdiff_exception, 0, "The diff file is corrupted (missing header information)");
        } else {
            zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to read data from the diff file");
        }
        goto cleanup;
    }

    /* Check for appropriate magic */
    if (memcmp(header, PHP_BSDIFF_MAGIC, 16) != 0) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "The diff file is corrupted (invalid header information)");
        goto cleanup;
    }

    /* Read lengths from header */
    newsize = offtin((uint8_t *)header + 16);
    if (newsize < 0 || (uint64_t)newsize >= (uint64_t)SIZE_MAX) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "The diff file is corrupted (invalid length information)");
        goto cleanup;
    }

    /* The output buffer is sized from the untrusted header, so check it against memory_limit up front. This turns
     * an oversized (or forged) length into a catchable exception instead of a fatal error from emalloc(). */
    if (PG(memory_limit) > 0 && (uint64_t)newsize >= (uint64_t)PG(memory_limit) - zend_memory_usage(0)) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "The patched file size (" ZEND_ULONG_FMT " bytes) exceeds the memory limit", (zend_ulong)newsize);
        goto cleanup;
    }

    /* Read old file into a managed zend_string */
    if (NULL == (old_str = php_bsdiff_read_file(old_file, "old", &old_mode))) {
        goto cleanup;
    }

    /* Allocate buffer for patched output; bspatch() writes into it */
    new_buf = emalloc((size_t)newsize + 1);

    memset(&reader.bz, 0, sizeof(reader.bz));
    reader.bz.bzalloc = php_bsdiff_bzalloc;
    reader.bz.bzfree = php_bsdiff_bzfree;
    reader.in = diff_s;
    reader.eof = 0;
    reader.stream_end = 0;
    reader.total = 0;
    reader.budget = (uint64_t)newsize <= (UINT64_MAX - 24) / 25 ? (uint64_t)newsize * 25 + 24 : UINT64_MAX;
    reader.status = PHP_BSDIFF_READ_OK;
    if ((bz2err = BZ2_bzDecompressInit(&reader.bz, 0, 0)) != BZ_OK) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to read data from the diff file (bz2err=%d)", bz2err);
        goto cleanup;
    }
    reader_initialized = 1;

    stream.read = bz2_read;
    stream.opaque = &reader;
    if (bspatch(
            (const uint8_t *)ZSTR_VAL(old_str),
            (int64_t)ZSTR_LEN(old_str),
            new_buf,
            newsize,
            &stream) ||
        php_bsdiff_reader_finish(&reader)) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "The diff file is corrupted (%s)", php_bsdiff_read_error(reader.status));
        goto cleanup;
    }

    /* Clean up the bzip2 reads and diff file before writing output */
    BZ2_bzDecompressEnd(&reader.bz);
    reader_initialized = 0;
    php_stream_close(diff_s);
    diff_s = NULL;

    /* Write the new file. Try to create it exclusively first, to know whether it is a new file (which gets the
     * permissions of the old file) or an existing one (which keeps its own permissions). Don't retry if open_basedir
     * refused the path (EPERM), which would only repeat its warning. */
    created = 1;
    errno = 0;
    new_s = php_stream_open_wrapper(new_file, "xb", 0, NULL);
    if (!new_s && errno != EPERM) {
        created = 0;
        new_s = php_stream_open_wrapper(new_file, "wb", 0, NULL);
    }
    if (!new_s) {
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to create the new file \"%s\"", new_file);
        goto cleanup;
    }

    /* Copy the permission bits (but not setuid/setgid/sticky) of the old file before writing any data. */
    if (created && old_mode >= 0) {
        php_bsdiff_chmod(new_file, old_mode);
    }

    if (newsize > 0 && (size_t)php_stream_write(new_s, (const char *)new_buf, (size_t)newsize) != (size_t)newsize) {
        php_stream_close(new_s);
        php_bsdiff_unlink(new_file);
        zend_throw_exception_ex(ce_bsdiff_exception, 0, "Failed to write to the new file \"%s\"", new_file);
        goto cleanup;
    }
    php_stream_close(new_s);

cleanup:
    if (reader_initialized) {
        BZ2_bzDecompressEnd(&reader.bz);
    }
    if (diff_s) {
        php_stream_close(diff_s);
    }
    if (new_buf) {
        efree(new_buf);
    }
    if (old_str) {
        zend_string_release(old_str);
    }
    if (EG(exception)) {
        RETURN_THROWS();
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
