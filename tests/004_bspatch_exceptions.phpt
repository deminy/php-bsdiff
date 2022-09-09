--TEST--
Test exceptions thrown from function bsdiff_patch()
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '004_old.out';
$new_file     = __DIR__ . DIRECTORY_SEPARATOR . '004_new.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '004_diff.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '004_patched.out';

foreach ([$old_file, $new_file, $diff_file, $patched_file] as $file) {
    if (file_exists($file)) unlink($file);
}

try {
    bsdiff_patch($old_file, $patched_file, $diff_file);
} catch (BsdiffException $e) {
    var_dump($e->getMessage());
}

touch($diff_file);
try {
    bsdiff_patch($old_file, $patched_file, $diff_file);
} catch (BsdiffException $e) {
    var_dump($e->getMessage());
}

file_put_contents($diff_file, "DEMINY/BSDIFF43**********");
try {
    bsdiff_patch($old_file, $patched_file, $diff_file);
} catch (BsdiffException $e) {
    var_dump($e->getMessage());
}

file_put_contents($old_file, str_repeat("Hello World", 1997));
file_put_contents($new_file, str_repeat("Hello PHP", 1999));
bsdiff_diff($old_file, $new_file, $diff_file);

unlink($old_file);
try {
    bsdiff_patch($old_file, $patched_file, $diff_file);
} catch (BsdiffException $e) {
    var_dump($e->getMessage());
}
file_put_contents($old_file, str_repeat("Hello World", 1997));

touch($patched_file);
chmod($patched_file, 0444);
try {
    bsdiff_patch($old_file, $patched_file, $diff_file);
} catch (BsdiffException $e) {
    var_dump($e->getMessage());
}
?>
--EXPECTF--
string(%d) "Cannot open diff file "%s004_diff.out" in read mode"
string(%d) "The diff file is corrupted (missing header information)"
string(%d) "The diff file is corrupted (invalid header information)"
string(%d) "Failed to open the old file "%s004_old.out""
string(%d) "Failed to create the new file "%s004_patched.out""
