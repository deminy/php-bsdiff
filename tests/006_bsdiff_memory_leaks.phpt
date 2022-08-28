--TEST--
Test memory usage
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

$mem0 = memory_get_usage();
file_put_contents($old_file, str_repeat("Hello World", 1997));
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
