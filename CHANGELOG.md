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
