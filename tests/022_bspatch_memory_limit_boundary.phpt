--TEST--
Test bsdiff_patch() reports output sizes close to memory_limit as exceptions, not fatal errors
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file     = __DIR__ . DIRECTORY_SEPARATOR . '022_old.out';
$diff_file    = __DIR__ . DIRECTORY_SEPARATOR . '022_diff.out';
$patched_file = __DIR__ . DIRECTORY_SEPARATOR . '022_patched.out';

file_put_contents($old_file, 'Hello World');

// One control entry that produces no output, so bzip2 allocates its decompressor state before the data runs out. This
// is bzcompress(str_repeat("\0", 24), 9), inlined so the test does not need ext/bz2.
$data = hex2bin('425a6839314159265359045383c5000000600040000400200021008283177245385090045383c5');

$messages = [];
for ($size = 1 << 20; $size <= 32 << 20; $size += 256 << 10) {
    file_put_contents($diff_file, 'ENDSLEY/BSDIFF43' . pack('P', $size) . $data);
    try {
        bsdiff_patch($old_file, $patched_file, $diff_file);
    } catch (BsdiffException $e) {
        $messages[preg_replace('/\(\d+ bytes\)/', '(N bytes)', $e->getMessage())] = true;
    }
}
echo implode(PHP_EOL, array_keys($messages)), PHP_EOL;
var_dump(file_exists($patched_file));
?>
--CLEAN--
<?php
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '022_old.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '022_diff.out');
@unlink(__DIR__ . DIRECTORY_SEPARATOR . '022_patched.out');
?>
--INI--
memory_limit=16M
--EXPECT--
The diff file is corrupted (unexpected end of data)
The patched file size (N bytes) exceeds the memory limit
bool(false)
