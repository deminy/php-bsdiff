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

// Set a memory limit low enough that allocating the old-file buffer
// (2 MB + 1 bytes) will exceed it.
ini_set('memory_limit', '2M');

bsdiff_patch($old_file, $patched_file, $diff_file);
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '011_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '011_new.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '011_diff.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '011_patched.out');
?>
--EXPECTF--
Fatal error: Allowed memory size of %d bytes exhausted (tried to allocate %d bytes) in %s on line %d
