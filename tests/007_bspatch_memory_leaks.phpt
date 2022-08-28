--TEST--
Test memory leaks in function bsdiff_patch()
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file     = __DIR__ . '/007_old.out';
$new_file     = __DIR__ . '/007_new.out';
$diff_file    = __DIR__ . '/007_diff.out';
$patched_file = __DIR__ . '/007_patched.out';

foreach ([$old_file, $new_file, $diff_file, $patched_file] as $file) {
    if (file_exists($file)) unlink($file);
}

file_put_contents($old_file, str_repeat("Hello World", 1997));
file_put_contents($new_file, str_repeat("Hello PHP", 1999));
bsdiff_diff($old_file, $new_file, $diff_file);

$mem0 = memory_get_usage();
touch($patched_file);
chmod($patched_file, 0444);
try {
    bsdiff_patch($old_file, $patched_file, $diff_file);
} catch (BsdiffException $e) {
    unset($e);
    $mem1 = memory_get_usage();
    var_dump($mem1 - $mem0);
}
?>
--EXPECT--
int(0)
