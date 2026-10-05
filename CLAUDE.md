# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A PHP extension (C) exposing `bsdiff_diff()` and `bsdiff_patch()` plus a `BsdiffException` class. Supports PHP 7.2 through 8.5 on Linux, macOS, and Windows. Requires libbz2. Distributed through PECL (`package.xml`) and PIE (`composer.json`).

## Build and test

```bash
phpize
./configure                      # add --with-bz2=DIR if bzlib.h is not auto-detected
make
make test                        # run all .phpt tests
make test TESTS="--show-diff tests"           # what CI runs
make test TESTS=tests/002_basic.phpt          # run a single test
```

`config.m4` looks for BZip2 in `/opt/homebrew/opt/bzip2`, `/usr/local/opt/bzip2`, `/opt/local`, `/usr/local`, and `/usr`. On macOS without Homebrew bzip2, CI uses `--with-bz2=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr`. On Ubuntu CI uses `--with-libdir=x86_64-linux-gnu`.

To clean generated build files: `phpize --clean` (or `make clean` for objects only).

CI (`.github/workflows/ci.yml`) builds and tests every PHP version from 7.2 to 8.5 on Ubuntu, macOS, and Windows (NTS and TS, via `php/php-windows-builder`). Windows builds use `config.w32`, so build changes must be made in both `config.m4` and `config.w32`.

## Architecture

- `bsdiff.c` / `bsdiff.h` / `bspatch.c` / `bspatch.h`: the bundled bsdiff library from https://github.com/mendsley/bsdiff (BSD-2-Clause). It works on in-memory buffers and talks to the outside world only through the `bsdiff_stream` / `bspatch_stream` callback structs. Keep changes here minimal. One local change: `offtout()` and `offtin()` are non-static so `php_bsdiff.c` can write and parse the patch header.
- `php_bsdiff.c`: the PHP binding. It reads input files into `zend_string`s with PHP streams, compresses or decompresses the diff data with libbz2's `bz_stream` API on top of the diff file's PHP stream, and passes read/write callbacks into the library. The patch format is the 16-byte magic `ENDSLEY/BSDIFF43`, an 8-byte new-file size, then the bzip2-compressed control/diff/extra data.
- `php_bsdiff.stub.php` is the source of truth for function signatures. `php_bsdiff_arginfo.h` is generated from it with php-src's `build/gen_stub.php`. Edit the stub and regenerate; do not hand-edit the arginfo header.
- `php_bsdiff.h` holds `PHP_BSDIFF_VERSION` and a `RETURN_THROWS()` shim for PHP < 8.0.

### Memory and resource conventions

These rules matter, and tests check them:

- All allocations, both the bsdiff library's and libbz2's, go through `emalloc`/`efree`. The library gets them via `stream.malloc`/`stream.free`; libbz2 gets them via `bzalloc`/`bzfree` on its low-level `bz_stream` API. They therefore count against `memory_limit` and show up in `memory_get_usage()` and PHP's leak detector, and PHP reclaims them if a fatal error (a longjmp) skips the cleanup code. Do not reintroduce libc `malloc`/`free`, or the `BZFILE` API, which allocates with `malloc`.
- All file I/O goes through `php_stream`s (`php_stream_read`/`php_stream_write`; no `FILE*`), so stream wrappers and `open_basedir` behave the same on every platform. Do not use raw `fopen` or path-based `VCWD_*` calls on user-supplied paths.
- Open files with explicit binary modes (`"rb"`/`"wb"`) for Windows.
- Each function uses one `cleanup:` label that releases everything and ends with `if (EG(exception)) RETURN_THROWS();`. Errors throw `BsdiffException` through `zend_throw_exception_ex(ce_bsdiff_exception, ...)` and then `goto cleanup`. A failed run removes its partially written output file if it created that file; in `bsdiff_diff()`, a `zend_try` block does this for fatal errors too.
- Everything in a diff file is untrusted. The output size in the header is checked against `SIZE_MAX` and (best effort) `memory_limit` before allocating; the bzip2 stream must end exactly where `bspatch()` stops reading; and the amount of data `bspatch()` may read is capped, which stops control data that produces no output.
- A local file written by `bsdiff_patch()`, new or existing, gets the old file's permission bits (`& 0777`), set through the file descriptor before any data is written.
- Output files are opened through `php_bsdiff_open_output()`: local files are created with `"xb"` first, so the code knows whether it created them, and only files it created are removed on failure. Other wrappers are opened with `"wb"`; they get no permission changes and are never removed.

### Tests

Tests are `.phpt` files in `tests/`. They include leak checks (the `memory_get_usage()` delta must be exactly `0`, after warming up stream internals), `memory_limit` exhaustion checks that expect a fatal error, and binary round-trips. Temporary files use the `.out` suffix (gitignored) and are removed in `--CLEAN--`.

## Release and packaging

- `package.xml` lists every shipped file explicitly. When you add or rename a source or test file, update `<contents>` there.
- A version bump changes `PHP_BSDIFF_VERSION` in `php_bsdiff.h`, the current release and `<notes>` in `package.xml` (and moves the previous release into `<changelog>`), and `CHANGELOG.md`.
- License: the extension is BSD-3-Clause and the bundled bsdiff library is BSD-2-Clause. Both texts are in `LICENSE`.
