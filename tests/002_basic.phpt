--TEST--
Basic test on bsdiff functions
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file     = __DIR__ . '/002_old.out';
$new_file     = __DIR__ . '/002_new.out';
$diff_file    = __DIR__ . '/002_diff.out';
$patched_file = __DIR__ . '/002_patched.out';

file_put_contents($old_file, str_repeat("Hello World", 1997));
file_put_contents($new_file, str_repeat("Hello PHP", 1999));

bsdiff_diff($old_file, $new_file, $diff_file);
bsdiff_patch($old_file, $patched_file, $diff_file);

var_dump(filesize($old_file));
var_dump(filesize($new_file));
var_dump(filesize($diff_file));
var_dump(filesize($patched_file));

var_dump(md5_file($old_file));
var_dump(md5_file($new_file));
var_dump(md5_file($diff_file));
var_dump(md5_file($patched_file));
?>
--EXPECT--
int(21967)
int(17991)
int(104)
int(17991)
string(32) "ebd4018bef6684280a19cbcb92099e4e"
string(32) "43a76cb50268ee3614012af8fac38ba0"
string(32) "041bfaaaf13b8cbde04b5ae614b096db"
string(32) "43a76cb50268ee3614012af8fac38ba0"
