--TEST--
bsdiff_patch() correctly reads back a diff file produced from binary input
--EXTENSIONS--
bsdiff
--FILE--
<?php
// Companion to 008_bsdiff_binary_roundtrip.phpt focused on the read side of
// the round-trip. Generates reproducible random payloads spanning the full
// 0x00..0xFF byte range, then verifies that the resulting diff file contains
// 0x0D, 0x0A and 0x1A bytes (the bytes most commonly mishandled by text-mode
// I/O on platforms that distinguish text and binary streams) and that
// bsdiff_patch() decodes it correctly.
//
// Note: this test cannot, by itself, prove that the C source opens the diff
// file in binary mode on Windows. Inside any PHP SAPI on Windows the global
// MSVC `_fmode` is forced to `_O_BINARY` at startup (see sapi/cli/php_cli.c
// and sapi/cgi/cgi_main.c in php-src), so fopen("...", "r") and
// fopen("...", "rb") behave identically when called from inside PHP.

$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '009_old.out';
$new_file     = __DIR__ . DIRECTORY_SEPARATOR . '009_new.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '009_diff.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '009_patched.out';

mt_srand(20260420);
$old = '';
$new = '';
for ($i = 0; $i < 8192; $i++) {
    $old .= chr(mt_rand(0, 255));
    $new .= chr(mt_rand(0, 255));
}
file_put_contents($old_file, $old);
file_put_contents($new_file, $new);

bsdiff_diff($old_file, $new_file, $diff_file);

// Sanity check: the diff stream contains the bytes that text-mode reads
// would mishandle on platforms that distinguish text and binary streams.
$diff_bytes = file_get_contents($diff_file);
var_dump(strpos($diff_bytes, "\r")   !== false);
var_dump(strpos($diff_bytes, "\n")   !== false);
var_dump(strpos($diff_bytes, "\x1A") !== false);

bsdiff_patch($old_file, $patched_file, $diff_file);

var_dump(filesize($patched_file) === filesize($new_file));
var_dump(md5_file($patched_file) === md5_file($new_file));

@unlink($old_file);
@unlink($new_file);
@unlink($diff_file);
@unlink($patched_file);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)

