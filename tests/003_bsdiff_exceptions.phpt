--TEST--
Test exceptions thrown from function bsdiff_diff()
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file  = __DIR__ . '/003_old.out';
$new_file  = __DIR__ . '/003_new.out';
$diff_file = __DIR__ . '/003_diff.out';

foreach ([$old_file, $new_file, $diff_file] as $file) {
    if (file_exists($file)) unlink($file);
}

try {
    bsdiff_diff($old_file, $new_file, $diff_file);
} catch (BsdiffException $e) {
    var_dump($e->getMessage());
}

file_put_contents($old_file, str_repeat("Hello World", 1997));
try {
    bsdiff_diff($old_file, $new_file, $diff_file);
} catch (BsdiffException $e) {
    var_dump($e->getMessage());
}

file_put_contents($new_file, str_repeat("Hello PHP", 1999));
touch($diff_file);
chmod($diff_file, 0444);
try {
    bsdiff_diff($old_file, $new_file, $diff_file);
} catch (BsdiffException $e) {
    var_dump($e->getMessage());
}
?>
--EXPECTF--
string(%d) "Failed to read data from the old file "%s/tests/003_old.out""
string(%d) "Failed to read data from the new file "%s/tests/003_new.out""
string(%d) "Cannot open the diff file "%s/tests/003_diff.out" in write mode"
