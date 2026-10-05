--TEST--
Test bsdiff_patch() does not change the permissions of files reached through symbolic or hard links
--EXTENSIONS--
bsdiff
--SKIPIF--
<?php
if (PHP_OS_FAMILY === 'Windows') die('skip POSIX permissions only');
?>
--FILE--
<?php
$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '023_old.out';
$new_file     = __DIR__ . DIRECTORY_SEPARATOR . '023_new.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '023_diff.out';
$target_file  = __DIR__ . DIRECTORY_SEPARATOR . '023_target.out';
$link_file    = __DIR__ . DIRECTORY_SEPARATOR . '023_link.out';

function mode(string $file): string
{
    clearstatcache();
    return sprintf('%o', fileperms($file) & 07777);
}

file_put_contents($old_file, str_repeat("Hello World", 1997));
file_put_contents($new_file, str_repeat("Hello PHP", 1999));
bsdiff_diff($old_file, $new_file, $diff_file);
chmod($old_file, 0777);

// A symbolic link: the target is written, but keeps its permissions.
file_put_contents($target_file, 'target');
chmod($target_file, 0600);
@unlink($link_file);
symlink($target_file, $link_file);
bsdiff_patch($old_file, $link_file, $diff_file);
echo 'symbolic link: ', mode($target_file), PHP_EOL;
var_dump(file_get_contents($target_file) === file_get_contents($new_file));

// A hard link: the file is written, but keeps its permissions.
unlink($link_file);
file_put_contents($target_file, 'target');
chmod($target_file, 0600);
link($target_file, $link_file);
bsdiff_patch($old_file, $link_file, $diff_file);
echo 'hard link: ', mode($target_file), PHP_EOL;
var_dump(file_get_contents($target_file) === file_get_contents($new_file));
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '023_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '023_new.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '023_diff.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '023_target.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '023_link.out');
?>
--EXPECT--
symbolic link: 600
bool(true)
hard link: 600
bool(true)
