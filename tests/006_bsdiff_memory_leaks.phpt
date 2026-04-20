--TEST--
Test memory leaks in function bsdiff_diff()
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file  = __DIR__ . DIRECTORY_SEPARATOR . '006_old.out';
$new_file  = __DIR__ . DIRECTORY_SEPARATOR . '006_new.out';
$diff_file = __DIR__ . DIRECTORY_SEPARATOR . '006_diff.out';

foreach ([$old_file, $new_file, $diff_file] as $file) {
    if (file_exists($file)) unlink($file);
}

// Create test files and warm up PHP stream read internals before measuring.
// The first php_stream_open_wrapper() call triggers one-time internal allocations (e.g.,
// resource hash table growth) that persist after the stream is closed. Warming up here
// ensures those allocations don't skew the measurement.
file_put_contents($old_file, str_repeat("Hello World", 1997));
$tmp = file_get_contents($old_file);
unset($tmp);

$mem0 = memory_get_usage();
try {
    bsdiff_diff($old_file, $new_file, $diff_file);
} catch (BsdiffException $e) {
    unset($e);
    $mem1 = memory_get_usage();
    var_dump($mem1 - $mem0);
}

file_put_contents($new_file, str_repeat("Hello PHP", 1999));
touch($diff_file);
chmod($diff_file, 0444);
// Warm up stream read path for new_file before measuring
$tmp = file_get_contents($new_file);
unset($tmp);

$mem2 = memory_get_usage();
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
