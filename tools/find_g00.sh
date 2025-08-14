#!/usr/bin/env bash
# find_g00.sh
# Find all g00 image files under a directory whose type equals a given hex value.
# Usage:  find_g00.sh <type-hex> [dir]

set -euo pipefail

if [[ ${1-} == "" ]]; then
  echo "Usage: $(basename "$0") <byte-hex> [dir]" >&2
  exit 2
fi

byte="$1"
dir="${2:-.}"

# Normalize the hex byte: remove optional 0x/0X and lowercase
byte="${byte#0x}"; byte="${byte#0X}"
byte=$(printf '%s' "$byte" | tr '[:upper:]' '[:lower:]')

# Validate: must be exactly two hex digits
if [[ ! "$byte" =~ ^[0-9a-f]{2}$ ]]; then
  echo "Error: byte must be two hex digits (e.g., 01, ff, 0x7f)." >&2
  exit 2
fi

# Use find and batch via -exec ... {} + so we spawn fewer shells.
# Read exactly 1 byte per file with od -N1 -tx1 and compare.
find -L "$dir" -type f -name "*.g00" -readable -exec sh -c '
  target="$1"; shift
  for f do
    # Read first byte as hex; suppress errors (e.g., permission denied).
    b=$(od -An -N1 -tx1 "$f" 2>/dev/null | tr -d "
")
    [ "$b" = "$target" ] && printf "%s
" "$f"
  done
' _ "$byte" {} +
