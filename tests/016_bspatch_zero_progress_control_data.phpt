--TEST--
Test bsdiff_patch() stops reading control data that produces no output
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '016_old.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '016_diff.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '016_patched.out';

file_put_contents($old_file, 'Hello World');

// One million control entries of (0, 0, 0): each one produces no output, and they compress to 50 bytes. This is
// bzcompress(str_repeat("\0", 24 * 1000000), 9), inlined so the test does not need ext/bz2.
$data = hex2bin('425a6839314159265359635de12800b7d2c010c00020000008200030cc09aa69929022daa29022f177245385090635de1280');
file_put_contents($diff_file, 'ENDSLEY/BSDIFF43' . pack('P', 1) . $data);

try {
    bsdiff_patch($old_file, $patched_file, $diff_file);
} catch (BsdiffException $e) {
    echo $e->getMessage(), PHP_EOL;
}
var_dump(file_exists($patched_file));
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '016_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '016_diff.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '016_patched.out');
?>
--EXPECT--
The diff file is corrupted (too much control data)
bool(false)
