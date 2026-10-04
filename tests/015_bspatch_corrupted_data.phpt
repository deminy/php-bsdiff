--TEST--
Test bsdiff_patch() rejects truncated diff files and diff files with extra data
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '015_old.out';
$new_file     = __DIR__ . DIRECTORY_SEPARATOR . '015_new.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '015_diff.out';
$bad_file     = __DIR__ . DIRECTORY_SEPARATOR . '015_bad.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '015_patched.out';

file_put_contents($old_file, str_repeat("Hello World", 1997));
file_put_contents($new_file, str_repeat("Hello PHP", 1999));
bsdiff_diff($old_file, $new_file, $diff_file);
$diff = file_get_contents($diff_file);

$cases = [];
foreach ([1, 4, 10, 20, 40] as $n) {
    $cases["truncated by {$n} bytes"] = substr($diff, 0, -$n);
}
$cases['trailing data'] = $diff . 'GARBAGE';
$cases['corrupted data'] = substr_replace($diff, 'XXXX', (int) (strlen($diff) / 2), 4);

foreach ($cases as $label => $data) {
    file_put_contents($bad_file, $data);
    try {
        bsdiff_patch($old_file, $patched_file, $bad_file);
        echo "{$label}: no exception", PHP_EOL;
    } catch (BsdiffException $e) {
        echo "{$label}: ", $e->getMessage(), PHP_EOL;
    }
}
var_dump(file_exists($patched_file));

bsdiff_patch($old_file, $patched_file, $diff_file);
var_dump(file_get_contents($patched_file) === file_get_contents($new_file));
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '015_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '015_new.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '015_diff.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '015_bad.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '015_patched.out');
?>
--EXPECTF--
truncated by 1 bytes: The diff file is corrupted (%s)
truncated by 4 bytes: The diff file is corrupted (%s)
truncated by 10 bytes: The diff file is corrupted (%s)
truncated by 20 bytes: The diff file is corrupted (%s)
truncated by 40 bytes: The diff file is corrupted (%s)
trailing data: The diff file is corrupted (unexpected extra data)
corrupted data: The diff file is corrupted (%s)
bool(false)
bool(true)
