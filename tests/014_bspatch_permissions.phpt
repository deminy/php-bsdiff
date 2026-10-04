--TEST--
Test file permissions of the file created by bsdiff_patch()
--EXTENSIONS--
bsdiff
--SKIPIF--
<?php
if (PHP_OS_FAMILY === 'Windows') die('skip POSIX permissions only');
?>
--FILE--
<?php
$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '014_old.out';
$new_file     = __DIR__ . DIRECTORY_SEPARATOR . '014_new.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '014_diff.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '014_patched.out';

function mode(string $file): string
{
    clearstatcache();
    return sprintf('%o', fileperms($file) & 07777);
}

file_put_contents($old_file, str_repeat("Hello World", 1997));
file_put_contents($new_file, str_repeat("Hello PHP", 1999));
bsdiff_diff($old_file, $new_file, $diff_file);

// A new file gets the permission bits of the old file, without setuid/setgid/sticky.
chmod($old_file, 04750);
@unlink($patched_file);
bsdiff_patch($old_file, $patched_file, $diff_file);
echo 'new file: ', mode($patched_file), PHP_EOL;
var_dump(file_get_contents($patched_file) === file_get_contents($new_file));

// An existing file keeps its own permissions.
chmod($patched_file, 0604);
bsdiff_patch($old_file, $patched_file, $diff_file);
echo 'existing file: ', mode($patched_file), PHP_EOL;

// Patching in place.
chmod($old_file, 0640);
copy($old_file, $patched_file);
chmod($patched_file, 0640);
bsdiff_patch($patched_file, $patched_file, $diff_file);
echo 'in place: ', mode($patched_file), PHP_EOL;
var_dump(file_get_contents($patched_file) === file_get_contents($new_file));

// file:// URLs work for all arguments.
@unlink($patched_file);
bsdiff_patch('file://' . $old_file, 'file://' . $patched_file, 'file://' . $diff_file);
echo 'file:// URLs: ', mode($patched_file), PHP_EOL;
var_dump(file_get_contents($patched_file) === file_get_contents($new_file));
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '014_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '014_new.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '014_diff.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '014_patched.out');
?>
--EXPECT--
new file: 750
bool(true)
existing file: 604
in place: 640
bool(true)
file:// URLs: 640
bool(true)
