--TEST--
Test memory leaks in function bsdiff_diff()
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file  = __DIR__ . '/006_old.out';
$new_file  = __DIR__ . '/006_new.out';
$diff_file = __DIR__ . '/006_diff.out';

foreach ([$old_file, $new_file, $diff_file] as $file) {
    if (file_exists($file)) unlink($file);
}

// It's kind of weird that when running tests in GitHub Actions, we have to put this call to function file_put_contents()
// before statement "$mem0 = memory_get_usage();", otherwise, the difference reported is "120" but not "0".
// However,
//   1. We don't need to make the same change for the 2nd test case in this file.
//   2. The issue doesn't happen when tested locally (with or without Docker).
file_put_contents($old_file, str_repeat("Hello World", 1997));

$mem0 = memory_get_usage();
try {
    bsdiff_diff($old_file, $new_file, $diff_file);
} catch (BsdiffException $e) {
    unset($e);
    $mem1 = memory_get_usage();
    var_dump($mem1 - $mem0);
}

$mem2 = memory_get_usage();
file_put_contents($new_file, str_repeat("Hello PHP", 1999));
touch($diff_file);
chmod($diff_file, 0444);
try {
    bsdiff_diff($old_file, $new_file, $diff_file);
} catch (BsdiffException $e) {
    unset($e);
    $mem3 = memory_get_usage();
    var_dump($mem3 - $mem2);
}
?>
--EXPECT--
int(0)
int(0)
