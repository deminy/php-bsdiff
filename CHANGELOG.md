## Unreleased

### Fixed

- `bsdiff_patch()`: reject output sizes in the diff header that do not fit in memory. This fixes a heap overflow on 32-bit platforms with crafted diff files, and turns an oversized length into a `BsdiffException` instead of a fatal error.
- `bsdiff_patch()`: verify that the compressed data ends exactly where the patch ends (including its CRC), so truncated diff files and diff files with trailing data are rejected.
- `bsdiff_patch()`: stop crafted diff files whose control data produces no output from running for an unbounded amount of time.
- Throw an exception when an input file is a directory or cannot be read completely, instead of treating it as an empty file.
- `bsdiff_patch()`: a newly created file gets the permission bits of the old file without setuid/setgid/sticky bits, set before any data is written; an existing file keeps its own permissions.
- `bsdiff_patch()`: get the permissions of the old file from the opened stream instead of a separate `stat()` call, so stream wrappers and `open_basedir` are respected.
- Remove the partially written output file when `bsdiff_diff()` or `bsdiff_patch()` fails, including when `bsdiff_diff()` exceeds the memory limit.
- Fix a memory leak of about 7.5 MB of bzip2 state when `bsdiff_diff()` exceeds the memory limit.

### Changed

- Use the low-level bzip2 API with `emalloc`/`efree`, so bzip2 memory counts against `memory_limit`.
- Read and write diff files through PHP streams only, so stream wrappers work for the diff file on all platforms.
- Error messages for corrupted diff data now give the reason, e.g., "The diff file is corrupted (unexpected end of data)". They replace "Failed to apply diff data".
- `bsdiff_patch()` now reports a missing old file as "Failed to open the old file" (previously "Failed to stat the old file").
- Windows: configuration fails with an error when BZip2 is not found, instead of failing later at link time.

## v0.2.1 (2026-06-24)

### Added

- Add `composer.json` for installation via [PIE] (PHP Installer for Extensions).

## v0.2.0 (2026-05-05)

### Changed

- Use `emalloc`/`efree` (via wrapper function pointers) for all internal bsdiff/bspatch allocations instead of libc `malloc`/`free`, so allocations respect `memory_limit` and are visible to PHP's leak detector and `memory_get_peak_usage()`.
- Replace POSIX file I/O with PHP streams and consolidate cleanup.
- Open diff files with explicit `"wb"`/`"rb"` mode flags to ensure binary-mode I/O on Windows regardless of the host's `_fmode` global.
- Improve BZip2 auto-detection in `config.m4`: search common Homebrew and system paths, and emit clearer error messages when headers or libraries are not found.

### Added

- PHP 8.3, 8.4, and 8.5 added to the CI matrix (Linux, macOS, Windows).

## v0.1.2 (2022-10-12)

### Changed

- Add PECL configuration option `with-bz2`.
- Include extension and BZip2 version numbers in `phpinfo()` output.

## v0.1.1 (2022-10-07)

Second public release.

This release is also available in [PECL].

## v0.1.0 (2022-08-29)

First public release.

[PECL]: https://pecl.php.net/package/bsdiff
