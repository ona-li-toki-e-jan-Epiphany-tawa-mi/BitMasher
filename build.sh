#!/bin/sh

# Error on unset variables.
set -u

CC="${CC:-cc}"
CFLAGS="${CFLAGS:--Wall -Wextra -Wpedantic -Wconversion -Wswitch-enum}"
EXTRA_CFLAGS="${EXTRA_CFLAGS:-}"
ALL_CFLAGS="$CFLAGS $EXTRA_CFLAGS -std=c11"

set -x
# shellcheck disable=SC2086 # We want $ALL_CFLAGS to wordsplit.
$CC $ALL_CFLAGS -o bitmasher bitmasher.c
