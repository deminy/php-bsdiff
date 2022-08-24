<?php

/** @generate-class-entries */

function bsdiff_diff(string $old_file, string $new_file, string $diff_file): bool {}

function bsdiff_patch(string $old_file, string $new_file, string $diff_file): bool {}

class BsdiffException extends Exception
{
}
