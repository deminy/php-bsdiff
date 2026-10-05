--TEST--
Test bsdiff_patch() works for small files under a small memory_limit
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '024_old.out';
$new_file     = __DIR__ . DIRECTORY_SEPARATOR . '024_new.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '024_diff.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '024_patched.out';

file_put_contents($old_file, str_repeat("Hello World", 1997));
file_put_contents($new_file, str_repeat("Hello PHP", 1999));
ini_set('memory_limit', '64M');
bsdiff_diff($old_file, $new_file, $diff_file);

ini_set('memory_limit', '8M');
bsdiff_patch($old_file, $patched_file, $diff_file);
var_dump(file_get_contents($patched_file) === file_get_contents($new_file));
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '024_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '024_new.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '024_diff.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '024_patched.out');
?>
--EXPECT--
bool(true)
