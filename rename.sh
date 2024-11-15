#!/bin/bash

# This script renames .cc files to .cpp and .h files to .hpp using git mv.

# Find all .cc and .h files recursively from the current directory
find . -type f \( -name '*.cc' -o -name '*.h' \) -print0 | while IFS= read -r -d '' file; do
    # Extract the directory and filename components
    dir="$(dirname "$file")"
    base="$(basename "$file")"
    name="${base%.*}"
    ext="${base##*.}"

    # Determine the new extension
    case "$ext" in
        cc)
            new_ext="cpp"
            ;;
        h)
            new_ext="hpp"
            ;;
        *)
            continue
            ;;
    esac

    # Construct the new file path
    new_file="$dir/$name.$new_ext"

    # Rename the file using git mv
    git mv "$file" "$new_file"
    echo "Renamed: $file -> $new_file"
done
