#!/usr/bin/python3

import sys
import os
import re

# Compile regular expressions for matching include guards
ifndef = re.compile('^\#ifndef')
define = re.compile('^\#define')
endif  = re.compile('^\#endif')

# Function to update include guards in a single file
def process_file(name):
  bak_name = name + ".bak"
  new_guard_name = name.replace('/', '_').replace('.', '_').upper() + "_"
  try:
    os.rename(name, bak_name)
    with open(bak_name, 'r') as input, open(name, 'w') as output:
      for line in input:
        if ifndef.search(line):
          output.write('#ifndef ' + new_guard_name + '\n')
          continue
        if define.search(line):
          output.write('#define ' + new_guard_name + '\n')
          continue
        if endif.search(line):
          output.write('#endif  // ' + new_guard_name + '\n')
          continue
        output.write(line)
  except IOError:
    sys.stderr.write(f'Cannot open "{name}"\n')

def main():
  directory = sys.argv[1]  # Get directory path from command line
  header_file_pattern = re.compile(r".*\.(h|hpp)$")

  # Iterate over all files in the specified directory
  for root, _, files in os.walk(directory):
    for file in files:
      if header_file_pattern.match(file):
        full_path = os.path.join(root, file)
        process_file(full_path)

if __name__ == "__main__":
  main()
