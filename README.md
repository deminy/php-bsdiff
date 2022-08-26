# php-bsdiff

`bsdiff` is a PHP extension to build and apply patches to binary files.

This PHP extension is based on [the bsdiff and bspatch libraries][1] maintained by Matthew Endsley. The original algorithm
and implementation was developed by Colin Percival. The algorithm is detailed in Colin's paper, [Naïve Differences of Executable Code][1].
For more information, visit his website at <http://www.daemonology.net/bsdiff/>.

---

## Requirements

* PHP 7.2 to PHP 8.x
* BZip2 1.0+

## Installation

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

## Functions

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

---

## License

MIT license.

[1]: https://github.com/mendsley/bsdiff
[2]: http://www.daemonology.net/papers/bsdiff.pdf
