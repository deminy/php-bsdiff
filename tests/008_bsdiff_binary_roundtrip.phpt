--TEST--
Round-trip bsdiff_diff()/bsdiff_patch() with random binary input (deterministic seed)
--EXTENSIONS--
bsdiff
--FILE--
<?php
// Complements 002_basic.phpt (which only exercises ASCII text). This test
// generates reproducible random payloads spanning the full 0x00..0xFF byte
// range so that the resulting BZip2 diff stream contains arbitrary binary
// bytes (NUL, CR, LF, 0x1A, etc.), then verifies that bsdiff_diff() and
// bsdiff_patch() round-trip the data byte-for-byte.
//
// Note: this test cannot, by itself, prove that the C source opens the diff
// file in binary mode on Windows. Inside any PHP SAPI on Windows the global
// MSVC `_fmode` is forced to `_O_BINARY` at startup (see sapi/cli/php_cli.c
// and sapi/cgi/cgi_main.c in php-src), so fopen("...", "w") and
// fopen("...", "wb") behave identically when called from inside PHP.

$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '008_old.out';
$new_file     = __DIR__ . DIRECTORY_SEPARATOR . '008_new.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '008_diff.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '008_patched.out';

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

// Sanity check: the BZip2-compressed diff stream contains LF (0x0A) bytes,
// confirming the payload is genuinely binary.
$diff_bytes = file_get_contents($diff_file);
var_dump(strpos($diff_bytes, "\n") !== false);

// The patched file must reproduce the new file byte-for-byte.
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

