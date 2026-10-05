--TEST--
Test bsdiff_patch() with diff files that have no compressed data
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '021_old.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '021_diff.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '021_patched.out';

file_put_contents($old_file, 'Hello World');

// A diff of an empty file needs no compressed data.
file_put_contents($diff_file, 'ENDSLEY/BSDIFF43' . "\0\0\0\0\0\0\0\0");
bsdiff_patch($old_file, $patched_file, $diff_file);
var_dump(file_get_contents($patched_file));

// Any other file does.
@unlink($patched_file);
file_put_contents($diff_file, 'ENDSLEY/BSDIFF43' . "\x01\0\0\0\0\0\0\0");
try {
    bsdiff_patch($old_file, $patched_file, $diff_file);
} catch (BsdiffException $e) {
    echo $e->getMessage(), PHP_EOL;
}
var_dump(file_exists($patched_file));
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '021_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '021_diff.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '021_patched.out');
?>
--EXPECT--
string(0) ""
The diff file is corrupted (unexpected end of data)
bool(false)
