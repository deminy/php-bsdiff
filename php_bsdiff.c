/* bsdiff extension for PHP */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"

#include "bsdiff_arginfo.h"
#include "bsdiff.h"
#include "bspatch.h"
#include "php_bsdiff.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <bzlib.h>
#include <err.h>
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

/* {{{ bool bsdiff_diff( string $oldFile, string $newFile, string $patchFile ) */
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

	/* Allocate oldsize+1 bytes instead of oldsize bytes to ensure
		that we never try to malloc(0) and get a NULL pointer */
	if(((fd=open(old_file,O_RDONLY,0))<0) ||
		((oldsize=lseek(fd,0,SEEK_END))==-1) ||
		((old=malloc(oldsize+1))==NULL) ||
		(lseek(fd,0,SEEK_SET)!=0) ||
		(read(fd,old,oldsize)!=oldsize) ||
		(close(fd)==-1)) err(1,"%s",old_file);


	/* Allocate newsize+1 bytes instead of newsize bytes to ensure
		that we never try to malloc(0) and get a NULL pointer */
	if(((fd=open(new_file,O_RDONLY,0))<0) ||
		((newsize=lseek(fd,0,SEEK_END))==-1) ||
		((new=malloc(newsize+1))==NULL) ||
		(lseek(fd,0,SEEK_SET)!=0) ||
		(read(fd,new,newsize)!=newsize) ||
		(close(fd)==-1)) err(1,"%s",new_file);

	/* Create the patch file */
	if ((pf = fopen(diff_file, "w")) == NULL) {
        err(1, "%s", diff_file);
    }

	/* Write header (signature+newsize)*/
	offtout(newsize, buf);
	if (fwrite("ENDSLEY/BSDIFF43", 16, 1, pf) != 1 ||
		fwrite(buf, sizeof(buf), 1, pf) != 1) {
        err(1, "Failed to write header");
    }


	if (NULL == (bz2 = BZ2_bzWriteOpen(&bz2err, pf, 9, 0, 0))) {
        errx(1, "BZ2_bzWriteOpen, bz2err=%d", bz2err);
    }

	stream.opaque = bz2;
	if (bsdiff(old, oldsize, new, newsize, &stream)) {
        err(1, "bsdiff");
    }

	BZ2_bzWriteClose(&bz2err, bz2, 0, NULL, NULL);
	if (bz2err != BZ_OK) {
        err(1, "BZ2_bzWriteClose, bz2err=%d", bz2err);
    }

	if (fclose(pf)) {
        err(1, "fclose");
    }

	/* Free the memory we used */
	free(old);
	free(new);

    RETURN_TRUE;
}
/* }}} */

/* {{{ bool bsdiff_patch( string $oldFile, string $newFile, string $patchFile ) */
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
        err(1, "fopen(%s)", diff_file);
    }

	/* Read header */
	if (fread(header, 1, 24, f) != 24) {
		if (feof(f)) {
            errx(1, "Corrupt patch\n");
        }
		err(1, "fread(%s)", diff_file);
	}

	/* Check for appropriate magic */
	if (memcmp(header, "ENDSLEY/BSDIFF43", 16) != 0) {
        errx(1, "Corrupt patch\n");
    }

	/* Read lengths from header */
	newsize=offtin(header+16);
	if(newsize<0) {
        errx(1,"Corrupt patch\n");
    }

	/* Close patch file and re-open it via libbzip2 at the right places */
	if(((fd=open(old_file,O_RDONLY,0))<0) ||
		((oldsize=lseek(fd,0,SEEK_END))==-1) ||
		((old=malloc(oldsize+1))==NULL) ||
		(lseek(fd,0,SEEK_SET)!=0) ||
		(read(fd,old,oldsize)!=oldsize) ||
		(fstat(fd, &sb)) ||
		(close(fd)==-1)) err(1,"%s",old_file);
	if((new=malloc(newsize+1))==NULL) err(1,NULL);

	if (NULL == (bz2 = BZ2_bzReadOpen(&bz2err, f, 0, 0, NULL, 0))) {
        errx(1, "BZ2_bzReadOpen, bz2err=%d", bz2err);
    }

	stream.read = bz2_read;
	stream.opaque = bz2;
	if (bspatch(old, oldsize, new, newsize, &stream)) {
        errx(1, "bspatch");
    }

	/* Clean up the bzip2 reads */
	BZ2_bzReadClose(&bz2err, bz2);
	fclose(f);

	/* Write the new file */
	if(((fd=open(new_file,O_CREAT|O_TRUNC|O_WRONLY,sb.st_mode))<0) ||
		(write(fd,new,newsize)!=newsize) || (close(fd)==-1)) {
        err(1,"%s",new_file);
    }

	free(new);
	free(old);

    RETURN_TRUE;
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
