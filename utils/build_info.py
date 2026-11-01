"""Update the repository's build_info.txt.

Usage:
    python utils/build_info.py --major 3
    python utils/build_info.py --minor 2 --patch 0
    python utils/build_info.py --major --incr
    python utils/build_info.py --minor --incr
    python utils/build_info.py --patch --incr --genbuild
    python utils/build_info.py --genbuild
    python utils/build_info.py --help

--major/--minor/--patch NUMBER set only the named fields.
--incr increments named fields supplied without a number.
--genbuild generates a new UUID, independently of version changes.
--ensure creates missing metadata without changing existing values (build tooling).
--file PATH selects another metadata file; the repository file is the default.
--cmake-output PATH writes CMake variables (build tooling).

This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.
"""

import argparse
from dataclasses import dataclass, replace
from pathlib import Path
import re
import sys
from uuid import UUID, uuid4


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
VERSION_FIELDS = ("major", "minor", "patch")


@dataclass(frozen=True)
class BuildInfo:
    major: int
    minor: int
    patch: int
    build_id: str

    def serialize(self):
        return (
            f"major={self.major}\nminor={self.minor}\npatch={self.patch}\n"
            f"build_id={self.build_id}\n"
        )


def read_build_info(path):
    fields = {}

    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue

        key, separator, value = line.partition("=")
        key, value = key.strip(), value.strip()

        if not separator or key in fields:
            raise ValueError(f"invalid or duplicate build-info field: {line!r}")

        fields[key] = value

    if fields.keys() != {*VERSION_FIELDS, "build_id"}:
        raise ValueError("build_info.txt must contain major, minor, patch and build_id")

    for key in VERSION_FIELDS:
        if not re.fullmatch(r"[0-9]+", fields[key]):
            raise ValueError(f"{key} must be a non-negative integer")

        fields[key] = int(fields[key])

    fields["build_id"] = str(UUID(fields["build_id"]))

    return BuildInfo(**fields)


def ensure_build_info(path):
    if not path.exists():
        initial = BuildInfo(1, 1, 0, str(uuid4()))
        path.parent.mkdir(parents=True, exist_ok=True)

        try:
            with path.open("x", encoding="utf-8", newline="\n") as stream:
                stream.write(initial.serialize())
        except FileExistsError:
            pass

    return read_build_info(path)


def write_if_changed(path, contents):
    if path.exists() and path.read_text(encoding="utf-8") == contents:
        return

    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(f".{path.name}.{uuid4().hex}.tmp")

    try:
        temporary.write_text(contents, encoding="utf-8", newline="\n")
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Update explicit version fields or generate a new engine build ID.",
        epilog="Examples: --major 3 | --minor --incr | --patch --incr --genbuild",
    )

    for name in VERSION_FIELDS:
        parser.add_argument(f"--{name}", nargs="?", const="increment", metavar="NUMBER",
                            help=f"set {name}; omit NUMBER when using --incr")

    parser.add_argument("--incr", action="store_true", help="increment fields supplied without NUMBER")
    parser.add_argument("--genbuild", action="store_true", help="generate a new build UUID")
    parser.add_argument("--ensure", action="store_true", help="create metadata only if it is missing")
    parser.add_argument("--file", type=Path, default=REPOSITORY_ROOT / "build_info.txt",
                        help="metadata file (default: repository build_info.txt)")
    parser.add_argument("--cmake-output", type=Path, help="write the resulting SFG_* CMake variables")
    args = parser.parse_args(argv)
    selected = {name: getattr(args, name) for name in VERSION_FIELDS if getattr(args, name) is not None}

    if not selected and not args.genbuild and not args.ensure:
        parser.error("specify --major, --minor, --patch, --genbuild or --ensure")

    if args.ensure and (selected or args.genbuild or args.incr):
        parser.error("--ensure cannot be combined with version/build changes")

    increments = [name for name, value in selected.items() if value == "increment"]

    if increments and not args.incr:
        parser.error("fields without NUMBER require --incr")

    if args.incr and not increments:
        parser.error("--incr requires --major, --minor or --patch without NUMBER")

    for name, value in selected.items():
        if value != "increment" and not re.fullmatch(r"[0-9]+", value):
            parser.error(f"--{name} requires a non-negative integer")

    try:
        info = ensure_build_info(args.file)
        updates = {
            name: getattr(info, name) + 1 if value == "increment" else int(value)
            for name, value in selected.items()
        }

        if args.genbuild:
            updates["build_id"] = str(uuid4())

        info = replace(info, **updates)
        write_if_changed(args.file, info.serialize())

        if args.cmake_output:
            contents = "".join(f"set(SFG_{name.upper()} {getattr(info, name)})\n" for name in VERSION_FIELDS)
            contents += f'set(SFG_BUILD "{info.build_id}")\n'
            write_if_changed(args.cmake_output, contents)

        print(info.serialize(), end="")
    except (OSError, ValueError) as error:
        print(f"build info: {error}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
