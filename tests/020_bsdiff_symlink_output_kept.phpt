--TEST--
Test bsdiff_diff() does not remove a symbolic link given as the diff file when it fails
--EXTENSIONS--
bsdiff
--SKIPIF--
<?php
if (PHP_OS_FAMILY === 'Windows') die('skip POSIX symbolic links only');
?>
--FILE--
<?php
$old_file = __DIR__ . DIRECTORY_SEPARATOR . '020_old.out';
$new_file = __DIR__ . DIRECTORY_SEPARATOR . '020_new.out';
$target   = __DIR__ . DIRECTORY_SEPARATOR . '020_target.out';
$link     = __DIR__ . DIRECTORY_SEPARATOR . '020_link.out';

file_put_contents($old_file, str_repeat("A", 524288));
file_put_contents($new_file, str_repeat("B", 524288));
file_put_contents($target, 'existing');
@unlink($link);
symlink($target, $link);

ini_set('memory_limit', '2M');

register_shutdown_function(function () use ($target, $link) {
    var_dump(is_link($link), file_exists($target));
});

bsdiff_diff($old_file, $new_file, $link);
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '020_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '020_new.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '020_target.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '020_link.out');
?>
--EXPECTF--
Fatal error: Allowed memory size of %d bytes exhausted (tried to allocate %d bytes) in %s on line %d
bool(true)
bool(true)
