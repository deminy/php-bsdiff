--TEST--
Test bsdiff_diff() respects PHP memory_limit
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file  = __DIR__ . DIRECTORY_SEPARATOR . '010_old.out';
$new_file  = __DIR__ . DIRECTORY_SEPARATOR . '010_new.out';
$diff_file = __DIR__ . DIRECTORY_SEPARATOR . '010_diff.out';

// Create 512 KB test files. The bsdiff algorithm internally allocates
// (oldsize+1)*8 bytes for the suffix array I, the same for V, plus
// newsize+1 for a working buffer — roughly 8.5 MB for 512 KB inputs.
file_put_contents($old_file, str_repeat("A", 524288));
file_put_contents($new_file, str_repeat("B", 524288));

// Set a memory limit low enough that the internal allocations will exceed it
// but high enough that PHP itself can still run.
ini_set('memory_limit', '2M');

bsdiff_diff($old_file, $new_file, $diff_file);
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '010_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '010_new.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '010_diff.out');
?>
--EXPECTF--
Fatal error: Allowed memory size of %d bytes exhausted (tried to allocate %d bytes) in %s on line %d
