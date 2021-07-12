## Name

comm - compare two sorted files line by line

## Synopsis

```**sh
$ comm [options...] <file1> <file2>
```

## Description

`comm` compares two **sorted** files specified by `file1` and `file2` line by line alphabetically. One of file1 and file2, but not both, can be `-`, in which case `comm` will read from `STDIN` for that file.

With no options, `comm` produces a three column output, offsetted by tabs (`\t`), of lines unique to `file1`, lines unique to `file2`, and lines common to both files. `comm` provides options to suppress the output of a specific column, use case insensitive comparison or print a summary.

## Options

* `-1`: Suppress the output of column 1 (lines unique to `file1`)
* `-2`: Suppress the output of column 2 (lines unique to `file2`)
* `-3`: Suppress the output of column 3 (lines common to `file1` and `file2`)
* `-i`: Use case insensitive comparison of lines
* `-c`, `--color`: Always print colored output even if `STDOUT` is not a tty
* `--no-color`: Do not print colored output
* `-t`, `--total`: Print a summary

## Arguments

* `file1`: First file to compare. Can be `-` for `STDIN`
* `file2`: Second file to compare. Can be `-` for `STDIN`

## Examples

```sh
# Files should be sorted first before using comm
$ sort < data1 > data1_sorted
$ sort < data2 > data2_sorted

# Display a three-column output of lines unique to data1,
# lines unique to data2 and lines common to both files
$ comm data1_sorted data2_sorted

# Pass one sorted file to STDIN and only display column 3
$ sort < data1 | comm -12c - data2_sorted | less

# Use case insensitive comparison, suppress output of all columns
# and print a summary
$ comm -123it data1_sorted data2_sorted
```
