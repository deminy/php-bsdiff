--TEST--
Check if bsdiff is loaded
--EXTENSIONS--
bsdiff
--FILE--
<?php
echo 'The extension "bsdiff" is available';
?>
--EXPECT--
The extension "bsdiff" is available
