# php-bsdiff

[![Build Status](https://github.com/deminy/php-bsdiff/actions/workflows/ci.yml/badge.svg)](https://github.com/deminy/php-bsdiff/actions/workflows/ci.yml)
[![PHP version](https://img.shields.io/badge/php-%3E%3D%207.2-8892BF.svg)](https://github.com/deminy/php-bsdiff)

`bsdiff` is a PHP extension to build and apply patches to binary files.

This PHP extension is based on [the bsdiff and bspatch libraries][1] maintained by Matthew Endsley. The original algorithm
and implementation was developed by Colin Percival. The algorithm is detailed in Colin's paper, [Naïve Differences of Executable Code][1].
For more information, visit his website at <http://www.daemonology.net/bsdiff/>.

---

## Requirements

* PHP 7.2 to PHP 8.x
* BZip2 1.0+
* Linux, macOS, or Windows

## Installation

## Install via PECL

```bash
pecl install bsdiff
```

In case BZip2 can't be found automatically, use option `--with-bz2` to specify the installation directory of BZip2. e.g.,

```bash
pecl install -D 'with-bz2="/usr/local/opt/bzip2"'
```

## Manual Installation

```bash
phpize
./configure
make
make test
make install
```

Once done, add the following line to your `php.ini` file:

```ini
extension=bsdiff.so
```

In case BZip2 can't be found automatically, use option `--with-bz2` to specify the installation directory of BZip2. e.g.,

```bash
./configure --with-bz2=/usr/local/opt/bzip2 # When BZip2 is installed via Homebrew on MacOS.
```

## Usage

There are two PHP functions added by the extension:

```php
/**
  * @throws \BsdiffException If there is any error happens.
  */
function bsdiff_diff(string $old_file, string $new_file, string $diff_file): void {}

/**
  * @throws \BsdiffException If there is any error happens.
  */
function bsdiff_patch(string $old_file, string $new_file, string $diff_file): void {}
```

Here is an example on how to use the extension:

```php
<?php
$old_file  = '/path/to/the/old/file';
$new_file  = '/path/to/the/new/file';
$diff_file = '/path/to/the/diff/file';

// To create the diff file.
try {
    bsdiff_diff($old_file, $new_file, $diff_file);
} catch (BsdiffException $e) {
    // Handle the exception.
}

// To apply the diff file.
$patched_file = '/path/to/the/patched/file';
try {
    bsdiff_patch($old_file, $patched_file, $diff_file);
} catch (BsdiffException $e) {
    // Handle the exception.
}
// File $patched_file will have exactly the same content as file $new_file.
```

---

## License

[The PHP license](LICENSE).

[1]: https://github.com/mendsley/bsdiff
[2]: http://www.daemonology.net/papers/bsdiff.pdf
