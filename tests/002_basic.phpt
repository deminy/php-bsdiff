--TEST--
Basic test on bsdiff functions
--EXTENSIONS--
bsdiff
--FILE--
<?php
$old_file     = __DIR__ . '/old.out';
$new_file     = __DIR__ . '/new.out';
$diff_file    = __DIR__ . '/diff.out';
$patched_file = __DIR__ . '/patched.out';

file_put_contents($old_file, str_repeat("Hello World", 1997));
file_put_contents($new_file, str_repeat("Hello PHP", 1999));

bsdiff_diff($old_file, $new_file, $diff_file);
bsdiff_patch($old_file, $patched_file, $diff_file);

var_dump(md5_file($diff_file));
var_dump(md5_file($new_file));
var_dump(md5_file($patched_file));
?>
--EXPECT--
string(32) "041bfaaaf13b8cbde04b5ae614b096db"
string(32) "43a76cb50268ee3614012af8fac38ba0"
string(32) "43a76cb50268ee3614012af8fac38ba0"
