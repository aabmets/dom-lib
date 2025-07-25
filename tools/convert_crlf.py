#
#   Apache License 2.0
#
#   Copyright (c) 2025, Mattias Aabmets
#
#   The contents of this file are subject to the terms and conditions defined in the License.
#   You may not use, modify, or distribute this file except in compliance with the License.
#
#   SPDX-License-Identifier: Apache-2.0
#

import sys
from pathlib import Path
import argparse


def convert_file(path: Path):
    try:
        data = path.read_bytes()
        new_data = data.replace(b'\r\n', b'\n')
        if new_data != data:
            path.write_bytes(new_data)
            print(f"Converted: {path}")
    except Exception as e:
        sys.exit(f"Error processing {path}: {e}")


def main(root_dir):
    for path in root_dir.rglob('*'):
        if path.is_file():
            convert_file(path)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Convert Windows CRLF to Unix LF recursively in files.'
    )
    parser.add_argument(
        'directory',
        nargs='?',
        default='.',
        help='Root directory to process (default: current directory)'
    )
    args = parser.parse_args()
    root = Path(args.directory)
    if not root.exists():
        sys.exit(f"The directory '{root}' does not exist.")
    main(root)
