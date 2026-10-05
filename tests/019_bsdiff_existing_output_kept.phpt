--TEST--
Test bsdiff_diff() does not remove an existing diff file when it fails
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file  = __DIR__ . DIRECTORY_SEPARATOR . '019_old.out';
$new_file  = __DIR__ . DIRECTORY_SEPARATOR . '019_new.out';
$diff_file = __DIR__ . DIRECTORY_SEPARATOR . '019_diff.out';

file_put_contents($old_file, str_repeat("A", 524288));
file_put_contents($new_file, str_repeat("B", 524288));
file_put_contents($diff_file, 'existing');

ini_set('memory_limit', '2M');

// Only files that bsdiff_diff() created itself are removed after a failure.
register_shutdown_function(function () use ($diff_file) {
    var_dump(file_exists($diff_file));
});

bsdiff_diff($old_file, $new_file, $diff_file);
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '019_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '019_new.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '019_diff.out');
?>
--EXPECTF--
Fatal error: Allowed memory size of %d bytes exhausted (tried to allocate %d bytes) in %s on line %d
bool(true)
