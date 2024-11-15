#!/usr/bin/env python3

import os
import re
import sys
import argparse

# Define the header file extensions you want to process
HEADER_EXTENSIONS = {'.h', '.hpp', '.hh', '.hxx', '.h++'}

def find_header_files(root_dir):
    """Recursively find all header files in the given directory."""
    header_files = []
    for dirpath, _, filenames in os.walk(root_dir):
        for filename in filenames:
            _, ext = os.path.splitext(filename)
            if ext.lower() in HEADER_EXTENSIONS:
                header_files.append(os.path.join(dirpath, filename))
    return header_files

def replace_header_guard(content):
    """
    Replace header guards with #pragma once, preserving any content before the header guard.
    """
    # Regex to match #ifndef GUARD and #define GUARD
    header_guard_pattern = re.compile(
        r'(^.*?)(^\s*#ifndef\s+(\w+)\s*$\n^\s*#define\s+\3\s*$\n?)',
        re.MULTILINE | re.DOTALL)

    # Search for header guard
    match = header_guard_pattern.search(content)
    if not match:
        return None  # No header guard found

    pre_guard_content = match.group(1)
    guard_start_pos = match.start(2)
    guard_end_pos = match.end(2)
    guard_name = match.group(3)

    # Now, find the matching #endif
    # We'll process the content starting from guard_end_pos
    post_define_content = content[guard_end_pos:]

    # Initialize the stack with our first #ifndef
    stack = ['#ifndef']

    # Regex to match preprocessor directives
    directive_pattern = re.compile(r'^\s*#(if|ifdef|ifndef|endif).*$', re.MULTILINE)

    # Find all directives in the content after #define
    for directive_match in directive_pattern.finditer(post_define_content):
        directive = directive_match.group(1)
        if directive in ('if', 'ifdef', 'ifndef'):
            stack.append('#' + directive)
        elif directive == 'endif':
            stack.pop()
            if not stack:
                # Found the matching #endif
                endif_pos = guard_end_pos + directive_match.end()
                # Extract the content between #define and #endif
                between_content = post_define_content[:directive_match.start()]
                post_endif_content = content[endif_pos:]
                # Reconstruct the content
                new_content = pre_guard_content + '\n#pragma once\n\n' + between_content + post_endif_content
                return new_content.rstrip() + '\n'
    # If we reach here, we didn't find the matching #endif
    return None

def process_file(file_path, dry_run=False, verbose=False):
    """Process a single file to replace header guards with #pragma once."""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading {file_path}: {e}", file=sys.stderr)
        return

    new_content = replace_header_guard(content)
    if new_content is None:
        if verbose:
            print(f"No header guard found in {file_path}. Skipping.")
        return

    if content == new_content:
        if verbose:
            print(f"No changes needed for {file_path}.")
        return

    if dry_run:
        print(f"[Dry Run] Would update: {file_path}")
        return

    try:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        if verbose:
            print(f"Updated: {file_path}")
    except Exception as e:
        print(f"Error writing to {file_path}: {e}", file=sys.stderr)

def main():
    parser = argparse.ArgumentParser(
        description="Replace header guards with #pragma once in all header files recursively."
    )
    parser.add_argument(
        'directory',
        nargs='?',
        default='.',
        help='Root directory to start processing (default: current directory)'
    )
    parser.add_argument(
        '-n', '--dry-run',
        action='store_true',
        help='Show which files would be modified without making changes'
    )
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Show detailed processing information'
    )
    args = parser.parse_args()

    root_dir = args.directory
    dry_run = args.dry_run
    verbose = args.verbose

    if not os.path.isdir(root_dir):
        print(f"Error: {root_dir} is not a valid directory.", file=sys.stderr)
        sys.exit(1)

    header_files = find_header_files(root_dir)
    if verbose:
        print(f"Found {len(header_files)} header file(s) to process.")

    for file_path in header_files:
        process_file(file_path, dry_run=dry_run, verbose=verbose)

    if verbose:
        print("Processing complete.")

if __name__ == '__main__':
    main()
