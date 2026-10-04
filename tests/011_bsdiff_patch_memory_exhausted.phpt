--TEST--
Test bsdiff_patch() respects PHP memory_limit
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '011_old.out';
$new_file     = __DIR__ . DIRECTORY_SEPARATOR . '011_new.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '011_diff.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '011_patched.out';

// Create 2 MB test files and generate a valid diff while we still have
// enough memory.
file_put_contents($old_file, str_repeat("A", 2097152));
file_put_contents($new_file, str_repeat("B", 2097152));
bsdiff_diff($old_file, $new_file, $diff_file);

// Set a memory limit low enough that allocating the output buffer
// (2 MB + 1 bytes) will exceed it. The size comes from the diff header, so
// it is checked up front and reported as an exception, not a fatal error.
ini_set('memory_limit', '2M');

try {
    bsdiff_patch($old_file, $patched_file, $diff_file);
} catch (BsdiffException $e) {
    echo $e->getMessage(), PHP_EOL;
}
var_dump(file_exists($patched_file));
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '011_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '011_new.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '011_diff.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '011_patched.out');
?>
--EXPECT--
The patched file size (2097152 bytes) exceeds the memory limit
bool(false)
