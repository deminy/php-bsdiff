--TEST--
Test bsdiff_diff() and bsdiff_patch() with stream filters and memory streams
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '018_old.out';
$new_file     = __DIR__ . DIRECTORY_SEPARATOR . '018_new.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '018_diff.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '018_patched.out';

$old = str_repeat("Hello World", 1997);
$new = str_repeat("Hello PHP", 1999);

// Input files read through a filter that makes the data shorter than the file on disk.
file_put_contents($old_file, base64_encode($old));
file_put_contents($new_file, $new);
$old_url = 'php://filter/read=convert.base64-decode/resource=' . $old_file;

bsdiff_diff($old_url, $new_file, $diff_file);
bsdiff_patch($old_url, $patched_file, $diff_file);
var_dump(file_get_contents($patched_file) === $new);

// Output to memory streams. Their contents are discarded, but writing them must succeed.
foreach (['php://memory', 'php://temp'] as $url) {
    bsdiff_diff($old_url, $new_file, $url);
    bsdiff_patch($old_url, $url, $diff_file);
    echo $url, ': OK', PHP_EOL;
}
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '018_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '018_new.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '018_diff.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '018_patched.out');
?>
--EXPECT--
bool(true)
php://memory: OK
php://temp: OK
