--TEST--
Test memory usage
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file     = __DIR__ . '/005_old.out';
$new_file     = __DIR__ . '/005_new.out';
$diff_file    = __DIR__ . '/005_diff.out';
$patched_file = __DIR__ . '/005_patched.out';

file_put_contents($old_file, str_repeat("Hello World", 1997));
file_put_contents($new_file, str_repeat("Hello PHP", 1999));

$i = 0;

$mem0 = memory_get_usage();

for (; $i <= 100; $i++) {
    if (file_exists($diff_file)) unlink($diff_file);
    if (file_exists($patched_file)) unlink($patched_file);

    bsdiff_diff($old_file, $new_file, $diff_file);
    bsdiff_patch($old_file, $patched_file, $diff_file);
}

$mem1 = memory_get_usage();
var_dump($mem1 - $mem0);
?>
--EXPECT--
int(0)
