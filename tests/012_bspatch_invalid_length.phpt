--TEST--
Test bsdiff_patch() rejects invalid or oversized lengths in the diff header
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '012_old.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '012_diff.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '012_patched.out';

file_put_contents($old_file, 'Hello World');

// The length is stored as sign-and-magnitude: the top bit of the last byte is the sign.
$sizes = [
    'negative'   => "\x01\0\0\0\0\0\0\x80",
    'INT64_MAX'  => pack('P', PHP_INT_MAX),
    '2^32'       => pack('P', 1 << 32),
    '2^32 - 1'   => pack('P', 0xFFFFFFFF),
    '1 GB'       => pack('P', 1 << 30),
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
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '012_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '012_diff.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '012_patched.out');
?>
--INI--
memory_limit=128M
--EXPECT--
negative: The diff file is corrupted (invalid length information)
INT64_MAX: The patched file size (N bytes) exceeds the memory limit
2^32: The patched file size (N bytes) exceeds the memory limit
2^32 - 1: The patched file size (N bytes) exceeds the memory limit
1 GB: The patched file size (N bytes) exceeds the memory limit
bool(false)
