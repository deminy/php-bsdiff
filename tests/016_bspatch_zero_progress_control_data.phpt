--TEST--
Test bsdiff_patch() stops reading control data that produces no output
--EXTENSIONS--
bsdiff
bz2
--FILE--
<?php
$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '016_old.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '016_diff.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '016_patched.out';

file_put_contents($old_file, 'Hello World');

// One million control entries of (0, 0, 0): each one produces no output, and they compress to a few hundred bytes.
file_put_contents($diff_file, 'ENDSLEY/BSDIFF43' . pack('P', 1) . bzcompress(str_repeat("\0", 24 * 1000000), 9));

try {
    bsdiff_patch($old_file, $patched_file, $diff_file);
} catch (BsdiffException $e) {
    echo $e->getMessage(), PHP_EOL;
}
var_dump(file_exists($patched_file));
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '016_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '016_diff.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '016_patched.out');
?>
--EXPECT--
The diff file is corrupted (too much control data)
bool(false)
