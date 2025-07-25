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

import re
import sys
import argparse
import textwrap
import functools
import typing as t
from pathlib import Path
from dataclasses import dataclass

__all__ = [
    "INCLUDE_PATTERN",
    "MACRO_PATTERN",
    "MACRO_PATTERN_MULTILINE",
    "CmdArgs",
    "Macro",
    "FileContents",
    "search_upwards",
    "extract_macro_definition",
    "parse_file_contents",
    "split_args",
    "substitute_params",
    "find_paren_pair",
    "expand_macros",
    "parse_args",
    "main",
]


INCLUDE_PATTERN = re.compile(
    r"#include\s*[\"'](?P<path>[^\"]*)[\"']"
)
MACRO_PATTERN = re.compile(
    r"#define\s*(?P<name>\w+)\s*\((?P<args>[\w\s,]+)\s*\)\s*(?P<expr>[^/\\]+)(?<!\s)"
)
MACRO_PATTERN_MULTILINE = re.compile(
    r"#define\s*(?P<name>\w+)\s*\((?P<args>[\w\s,]+)\s*\)\s*\n(?P<expr>[^/\\]+)(?<!\s)"
)


class CmdArgs(argparse.Namespace):
    process_path: Path
    output_dir: Path


@dataclass
class Macro:
    name: str
    args: list[str]
    expr: str


@dataclass
class FileContents:
    lines: list[str]
    imports: list[Path]
    macros: dict[str, Macro]


@functools.cache
def search_upwards(for_path: Path | str, from_path: Path | str = __file__) -> Path:
    current = Path(from_path).resolve()
    while current != current.parent:
        candidate = current / for_path
        if candidate.exists():
            return candidate
        current = current.parent
    raise FileNotFoundError(f"Cannot locate '{for_path}' above {from_path}.")


def extract_macro_definition(lines: list[str], index: int) -> Macro | None:
    pattern = MACRO_PATTERN
    line = lines.pop(index)

    if line.endswith("\\"):
        pattern = MACRO_PATTERN_MULTILINE
        while True:
            next_line = lines.pop(index)
            line = line.rstrip('\\ ') + '\n' + next_line
            if not next_line.endswith("\\"):
                break

    match = pattern.match(line)
    if not match:
        return None

    name = match.group('name')
    args = match.group('args')
    expr = match.group('expr')
    expr = textwrap.dedent(expr)

    if not any([name, args, expr]):
        raise RuntimeError(f"Could not parse macro definition:\n{line}")

    args = [x.strip() for x in args.split(',')]
    return Macro(name, args, expr)


def parse_file_contents(file_path: Path) -> FileContents:
    lines: list[str] = file_path.read_text(encoding="utf-8").splitlines()
    src_dir: Path = search_upwards("src")
    file_imports: list[Path] = []
    macros: dict[str, Macro] = {}
    index = 0

    while index < len(lines):
        line = lines[index]
        prev_line = lines[max(index - 1, 0)]

        if line == '' and prev_line == '':
            lines.pop(index)
            continue

        elif line.startswith("//"):
            lines.pop(index)
            continue

        elif "#include" in line:
            if match := INCLUDE_PATTERN.match(line):
                import_path = src_dir / match.group('path')
                file_imports.append(import_path)

        elif "#define" in line:
            if macro := extract_macro_definition(lines, index):
                macros[macro.name] = macro
                continue

        index += 1

    return FileContents(lines, file_imports, macros)


def split_args(arg_string: str) -> list[str]:
    args, current, depth = [], "", 0
    for ch in arg_string:
        if ch == "(":
            depth += 1
            current += ch
        elif ch == ")":
            if depth == 0:
                break
            depth -= 1
            current += ch
        elif ch == "," and depth == 0:
            args.append(current.strip())
            current = ""
        else:
            current += ch
    if current.strip():
        args.append(current.strip())
    return args


def substitute_params(expr: str, formals: list[str], actual_args: list[str]) -> str:
    for formal, actual in zip(formals, actual_args):
        expr = re.sub(rf'(?<!#)#\s*{re.escape(formal)}\b', f'"{actual}"', expr)
    for formal, actual in zip(formals, actual_args):
        expr = re.sub(rf'\b{re.escape(formal)}\b', actual, expr)
    return expr


def find_paren_pair(line: str, match: re.Match) -> tuple[int, int]:
    open_idx = line.find("(", match.start())
    depth, idx = 0, open_idx + 1
    while idx < len(line):
        if line[idx] == "(":
            depth += 1
        elif line[idx] == ")":
            if depth == 0:
                return open_idx, idx
            depth -= 1
        idx += 1
    raise RuntimeError(f"Unmatched parentheses on line:\n{line}")


def expand_macros(
        line: str,
        pattern: re.Pattern,
        macros: dict[str, Macro],
        _depth: int = 0,
        _max_depth: int = 50
) -> str:
    if _depth > _max_depth:
        return line

    while True:
        match = pattern.search(line)
        if not match:
            return line
        name = match.group(1)
        macro = macros.get(name)
        if macro is None:
            raise RuntimeError(f"Macro '{name}' is not defined")

        open_idx, close_idx = find_paren_pair(line, match)
        arg_str = line[open_idx + 1 : close_idx]
        actual_args = [
            expand_macros(a, pattern, macros, _depth + 1)
            for a in split_args(arg_str)
        ]
        if len(actual_args) != len(macro.args):
            expected, actual = len(macro.args), len(actual_args)
            raise RuntimeError(f"Macro '{name}' expects {expected} args, got {actual}")

        expr = substitute_params(macro.expr, macro.args, actual_args)
        expr = re.sub(r"\s*##\s*", "", expr)
        expr = expand_macros(expr, pattern, macros, _depth + 1)

        line = line[: match.start()] + expr + line[close_idx + 1 :]


def parse_args() -> CmdArgs:
    src_dir = search_upwards("src")

    def process_path(path_str: str, must_be_dir: bool = False) -> Path:
        path = Path(path_str)
        if not path.is_absolute():
            path = (src_dir / path)
        path = path.resolve()
        if not path.exists():
            raise RuntimeError(f"Cannot locate path: '{path}'")
        elif path.is_file():
            if must_be_dir:
                raise RuntimeError(f"Path is not a directory: '{path}'")
            elif path.suffix not in ['.c', '.h']:
                raise RuntimeError(f"Cannot process file: '{path}'")
        return path

    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-p', "--process-path",
        required=False,
        default=src_dir,
        type=process_path,
        help="Path to a directory or a file to process. If not specified, "
             "all paths in the src directory are recursively processed."
    )
    parser.add_argument(
        '-o', "--output-dir",
        required=False,
        default=src_dir / ".preprocessed",
        type=process_path,
        help="Path to the output directory of the processed files. "
             "If not specified, the files will be written to the "
             "'src/.preprocessed' directory."
    )
    return t.cast(CmdArgs, parser.parse_args())


def main():
    args = parse_args()
    src_dir = search_upwards("src")
    preprocessing_targets: dict[Path, FileContents] = {}
    imported_dependencies: dict[Path, FileContents] = {}

    # Initial import of target files to preprocess
    if args.process_path.is_file():
        if not args.process_path.is_relative_to(args.output_dir):
            preprocessing_targets[args.process_path] = parse_file_contents(args.process_path)
    else:
        for file in args.process_path.rglob("*.[ch]"):
            if not file.is_relative_to(args.output_dir):
                preprocessing_targets[file] = parse_file_contents(file)

    # Exit early if all targets were relative to the output directory
    if not preprocessing_targets:
        sys.exit("No files to preprocess. Exiting.")

    # Resolve import dependencies
    for file, contents in preprocessing_targets.items():
        for import_path in contents.imports:
            if import_path in preprocessing_targets:
                other_file = preprocessing_targets[import_path]

            elif import_path in imported_dependencies:
                other_file = imported_dependencies[import_path]

            else:
                other_file = parse_file_contents(import_path)
                imported_dependencies[import_path] = other_file

            # Populate target file macros from dependencies
            for name, macro in other_file.macros.items():
                contents.macros[name] = macro

    # Expand macros
    for file, contents in preprocessing_targets.items():
        if not contents.macros:
            continue
        sanitized_names = [re.escape(name) for name in contents.macros.keys()]
        macro_names = '|'.join(sorted(sanitized_names, key=len, reverse=True))
        pattern = re.compile(rf'\b({macro_names})\s*\(')

        for index, line in enumerate(contents.lines):
            try:
                expanded = expand_macros(line, pattern, contents.macros)
            except RuntimeError as e:
                print(macro_names)
                print(pattern)
                raise e
            if expanded != line:
                expanded += '\n'
            contents.lines[index] = expanded

    # Write preprocessed files to the output directory
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for file, contents in preprocessing_targets.items():
        rel_path = file.relative_to(src_dir)
        out_file = args.output_dir / rel_path
        out_file.parent.mkdir(parents=True, exist_ok=True)
        out_file.write_text("\n".join(contents.lines), encoding="utf-8")
        out_rel = out_file.relative_to(src_dir)
        print(f"Preprocessed '{rel_path}' to '{out_rel}'")


if __name__ == '__main__':
    main()
