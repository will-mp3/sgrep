# sgrep
## Simple Grep implementation using C
### NOTE: sgrep implementation makes use of the Mu.c file for many of its functions. This helper file was not created by myself, all credit to original authors.
sgrep includes two text files for use, though any text file may be used alongside the code.

## functionality includes:

### -c, --count
Suppress normal output; instead print a count of matching lines for the input file. With the -v option, count non-matching lines.

### -h, --help
Print a usage statement to stdout and exit with status 0.

### -n, --line-number
Prefix each line of output with the 1-based line number of the file, followed immediately by a colon (e.g., 1:foo).

### -q, --quiet
Quiet; do not write anything to stdout. Exit immediately with zero status if any match was found. If a match is not found, exit with a non-zero status.

### -B NUM, --before-context NUM
Print NUM lines of leading context before matching lines.
