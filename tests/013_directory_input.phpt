--TEST--
Test directories passed as input files are rejected
--EXTENSIONS--
bsdiff
--SKIPIF--
<?php
if (PHP_OS_FAMILY === 'Windows') die('skip directories cannot be opened as files on Windows');
?>
--FILE--
<?php
$dir          = __DIR__ . DIRECTORY_SEPARATOR . '013_dir.out';
$file         = __DIR__ . DIRECTORY_SEPARATOR . '013_file.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '013_diff.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '013_patched.out';

@mkdir($dir);
file_put_contents($file, str_repeat("Hello World", 1997));

foreach ([[$dir, $file], [$file, $dir]] as [$old, $new]) {
    try {
        bsdiff_diff($old, $new, $diff_file);
    } catch (BsdiffException $e) {
        echo str_replace(__DIR__ . DIRECTORY_SEPARATOR, '', $e->getMessage()), PHP_EOL;
    }
}
var_dump(file_exists($diff_file));

bsdiff_diff($file, $file, $diff_file);
try {
    bsdiff_patch($dir, $patched_file, $diff_file);
} catch (BsdiffException $e) {
    echo str_replace(__DIR__ . DIRECTORY_SEPARATOR, '', $e->getMessage()), PHP_EOL;
}
var_dump(file_exists($patched_file));
?>
--CLEAN--
<?php
@rmdir(__DIR__ . DIRECTORY_SEPARATOR . '013_dir.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '013_file.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '013_diff.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '013_patched.out');
?>
--EXPECT--
The old file "013_dir.out" is a directory
The new file "013_dir.out" is a directory
bool(false)
The old file "013_dir.out" is a directory
bool(false)
