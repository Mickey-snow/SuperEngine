#!/usr/bin/env bash
# find_pdt10_magic.sh
# Recursively list regular files whose FIRST 5 BYTES are the ASCII string "PDT10".
# Usage:
#   find_pdt10_magic.sh [DIR]
# Examples:
#   find_pdt10_magic.sh           # search current directory
#   find_pdt10_magic.sh /path/to/dir

set -u  # treat unset variables as error

DIR=${1:-.}

# Use find in batched mode; for each file read first 5 bytes and compare to PDT10
find -L "$DIR" -type f -readable -exec sh -c '
  for f do
    bytes=""
    IFS= read -rN5 bytes < "$f" 2>/dev/null || true
    [ "$bytes" = "PDT11" ] && printf "%s
" "$f"
  done
' _ {} +
