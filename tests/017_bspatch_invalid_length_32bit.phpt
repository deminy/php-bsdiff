--TEST--
Test bsdiff_patch() rejects lengths in the diff header that do not fit in memory on 32-bit platforms
--EXTENSIONS--
bsdiff
--SKIPIF--
<?php
if (PHP_INT_SIZE !== 4) die('skip 32-bit only');
?>
--FILE--
<?php
$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '017_old.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '017_diff.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '017_patched.out';

file_put_contents($old_file, 'Hello World');

// Little-endian lengths, written out byte by byte because 32-bit PHP cannot pack 64-bit integers.
$sizes = [
    'INT64_MAX'  => "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x7F",
    '2^32'       => "\0\0\0\0\x01\0\0\0",
    '2^32 - 1'   => "\xFF\xFF\xFF\xFF\0\0\0\0",
    '1 GB'       => "\0\0\0\x40\0\0\0\0",
];

foreach ($sizes as $label => $size) {
    file_put_contents($diff_file, 'ENDSLEY/BSDIFF43' . $size . 'BZh9');
    try {
        bsdiff_patch($old_file, $patched_file, $diff_file);
        echo "{$label}: no exception", PHP_EOL;
    } catch (BsdiffException $e) {
        echo "{$label}: ", preg_replace('/\(\d+ bytes\)/', '(N bytes)', $e->getMessage()), PHP_EOL;
    }
}
var_dump(file_exists($patched_file));
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '017_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '017_diff.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '017_patched.out');
?>
--INI--
memory_limit=128M
--EXPECT--
INT64_MAX: The diff file is corrupted (invalid length information)
2^32: The diff file is corrupted (invalid length information)
2^32 - 1: The diff file is corrupted (invalid length information)
1 GB: The patched file size (N bytes) exceeds the memory limit
bool(false)
