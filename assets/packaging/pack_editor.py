"""Synchronize editor assets, SDK and runtime binaries using CMake's manifest.

    python assets/packaging/pack_editor.py --manifest _build/packaging/Debug/sdk_build.json
    python assets/packaging/pack_editor.py --help

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
from dataclasses import asdict
import filecmp
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import sys
import zipfile
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "utils"))

from build_info import ensure_build_info


HEADER_SUFFIXES = {".h", ".hpp", ".hxx", ".inl", ".ipp", ".inc", ".def"}
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx"}
LICENSE_NAMES = {"license", "license.txt", "license-mit.txt", "copying", "copying.txt", "notice", "copyright"}


def file_hash(path):
    digest = hashlib.sha256()

    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)

    return digest.hexdigest()


def file_crc(path):
    result = 0

    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            result = zlib.crc32(block, result)

    return result


def writable(path):
    if path.exists():
        path.chmod(path.stat().st_mode | stat.S_IWRITE)


def validate_destination(root, path):
    if not path.resolve().is_relative_to(root.resolve()):
        raise ValueError(f"SDK destination escapes its bundle: {path}")

    if path.exists() and (path.is_symlink() or getattr(path.lstat(), "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT):
        raise ValueError(f"SDK packaging does not follow links or junctions: {path}")


def existing_files(root):
    files = []
    directories = []
    validate_destination(root.parent, root)

    for directory, names, filenames in os.walk(root, followlinks=False):
        current = Path(directory)

        for name in names:
            child = current / name

            if child.lstat().st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT:
                raise ValueError(f"SDK packaging does not follow links or junctions: {child}")

            directories.append(child)

        for name in filenames:
            child = current / name

            if child.lstat().st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT:
                raise ValueError(f"SDK packaging does not follow links or junctions: {child}")

            files.append(child)

    return files, directories


def synchronize(root, desired, archive):
    files, directories = existing_files(root)
    copied = 0
    removed = 0

    for relative, source in desired.items():
        relative_path = PurePosixPath(relative)

        if relative_path.is_absolute() or ".." in relative_path.parts or ":" in relative or "\\" in relative:
            raise ValueError(f"invalid SDK destination: {relative}")

        destination = root / relative
        destination.parent.mkdir(parents=True, exist_ok=True)

        if isinstance(source, Path):
            unchanged = destination.is_file() and filecmp.cmp(source, destination, shallow=False)
        elif isinstance(source, zipfile.ZipInfo):
            unchanged = destination.is_file() and destination.stat().st_size == source.file_size and file_crc(destination) == source.CRC
        else:
            unchanged = destination.is_file() and destination.read_bytes() == source

        if not unchanged:
            writable(destination)

            if isinstance(source, Path):
                shutil.copyfile(source, destination)
            elif isinstance(source, zipfile.ZipInfo):
                with archive.open(source) as input_stream, destination.open("wb") as output_stream:
                    shutil.copyfileobj(input_stream, output_stream)
            else:
                destination.write_bytes(source)

            copied += 1

        if relative.startswith("stakeforge_sdk/include/"):
            mode = destination.stat().st_mode

            if mode & (stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH):
                destination.chmod(mode & ~(stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH))

    for path in files:
        if path.relative_to(root).as_posix() not in desired:
            writable(path)
            path.unlink()
            removed += 1

    for path in reversed(directories):
        if not any(path.iterdir()):
            path.rmdir()

    return copied, removed


def sdk_config(settings, definitions, options, flags):
    root = "${_stakeforge_sdk_root}"
    libraries = [f"{root}/lib/{Path(path).name}" for path in settings["dependency_libraries"]]
    libraries.extend(settings["system_libraries"])
    replacements = {
        "VERSION": ".".join(str(settings["build_info"][field]) for field in ("major", "minor", "patch")),
        "BUILD_ID": settings["build_info"]["build_id"],
        "CONFIGURATION": settings["configuration"],
        "CONFIGURATION_UPPER": settings["configuration"].upper(),
        "COMPILER_ID": settings["compiler_id"],
        "COMPILER_VERSION": settings["compiler_version"],
        "POINTER_SIZE": str(settings["pointer_size"]),
        "ARCHITECTURE": settings["architecture"],
        "TOOLSET": settings["toolset"],
        "WINDOWS_SDK": settings["windows_sdk"],
        "MSVC_RUNTIME": settings["msvc_runtime"],
        "CORE_LIBRARY": Path(settings["core_import_library"]).name,
        "TOOLS_LIBRARY": Path(settings["tools_import_library"]).name,
        "INCLUDE_DIRECTORIES": ";".join(f"{root}/{path}" for path in settings["include_directories"]),
        "DEFINITIONS": ";".join(definitions).replace('"', '\\"'),
        "OPTIONS": ";".join(option for option in options if not re.fullmatch(r"[/\-](?:O[a-zA-Z0-9-]+|GL-?|Zi|ZI|Z7)", option)).replace('"', '\\"'),
        "LIBRARIES": ";".join(libraries),
        **{key.upper(): re.sub(
            r"(?<!\S)[/\-](?:O[a-zA-Z0-9-]+|GL-?|Zi|ZI|Z7)(?=\s|$)" if key.startswith("cxx_")
            else r"(?i)(?<!\S)[/\-](?:DEBUG(?::\S+)?|INCREMENTAL(?::\S+)?|OPT:\S+|LTCG(?::\S+)?)(?=\s|$)",
            "", value).strip() for key, value in flags.items()},
    }
    contents = Path(__file__).with_name("StakeforgeConfig.cmake.in").read_text(encoding="utf-8")

    for key, value in replacements.items():
        contents = contents.replace(f"@{key}@", value)

    return contents.encode("utf-8")


def package(manifest):
    settings = json.loads(manifest.read_text(encoding="utf-8"))
    source_root = Path(settings["source_root"]).resolve()
    output_root = Path(settings["output_root"]).resolve()
    assets_root = output_root / "assets"
    info = ensure_build_info(source_root / "build_info.txt")

    if asdict(info) != settings["build_info"]:
        raise ValueError("build_info.txt changed since configuration; rerun CMake and rebuild before packaging")

    definitions = list(dict.fromkeys(line for line in manifest.with_name("definitions.txt").read_text().splitlines() if line))
    options = list(dict.fromkeys(line for line in manifest.with_name("options.txt").read_text().splitlines() if line))
    flag_names = ("cxx_flags", "cxx_config_flags", "shared_linker_flags", "shared_linker_config_flags", "exe_linker_flags", "exe_linker_config_flags")
    flags = dict(zip(flag_names, manifest.with_name("flags.txt").read_text().splitlines(), strict=True))
    cmake_source = source_root / "assets/packaging/cmake"
    distribution = json.loads((cmake_source / "distribution.json").read_text(encoding="utf-8"))
    archive_path = cmake_source / distribution["archive"]

    if file_hash(archive_path) != distribution["sha256"]:
        raise ValueError(f"portable CMake checksum mismatch: {archive_path}")

    desired = {}

    for source, target in settings["headers"]:
        directory = Path(source)
        suffixes = HEADER_SUFFIXES | SOURCE_SUFFIXES if target == "include/sfg" else HEADER_SUFFIXES

        if not directory.is_dir():
            raise FileNotFoundError(f"SDK header directory is missing: {directory}")

        for path in sorted(directory.rglob("*")):
            if path.is_file() and (path.suffix.lower() in suffixes or path.name.lower() in LICENSE_NAMES):
                desired[f"{target}/{path.relative_to(directory).as_posix()}"] = path

    for source in [settings["core_import_library"], settings["tools_import_library"], *settings["dependency_libraries"]]:
        path = Path(source)

        if not path.is_file():
            raise FileNotFoundError(f"SDK library is missing; build core/tools first: {path}")

        desired[f"lib/{path.name}"] = path

    runtime_files = [Path(source) for source in settings["runtime_files"] if source]

    for path in runtime_files:
        if not path.is_file():
            raise FileNotFoundError(f"SDK runtime file is missing; build core/tools first: {path}")

    for name in ("LICENSE", "GAME-LINKING-EXCEPTION.md"):
        desired[f"licenses/{name}"] = source_root / name

    for directory in (source_root / "stakeforge_core/deps").iterdir():
        if directory.is_dir():
            for path in directory.iterdir():
                if path.is_file() and path.name.lower() in LICENSE_NAMES:
                    desired[f"licenses/{directory.name}/{path.name}"] = path

    for name in ("FTL.TXT", "GPLv2.TXT"):
        desired[f"licenses/freetype-2.14.3/{name}"] = source_root / "stakeforge_core/deps/freetype-2.14.3/docs" / name

    desired["build_info.txt"] = info.serialize().encode("utf-8")
    desired["cmake/StakeforgeConfig.cmake"] = sdk_config(settings, definitions, options, flags)
    metadata = {key: settings[key] for key in (
        "build_info", "configuration", "generator", "architecture", "pointer_size", "compiler_id", "compiler_version",
        "toolset", "toolset_version", "windows_sdk", "msvc_runtime", "debug_information_format",
    )}
    metadata.update({"sdk_format": 1, "cmake_version": distribution["version"], "definitions": definitions, "options": options, "flags": flags})
    desired["build_settings.json"] = (json.dumps(metadata, indent=4) + "\n").encode("utf-8")

    with zipfile.ZipFile(archive_path) as archive:
        for entry in archive.infolist():
            path = PurePosixPath(entry.filename)

            if path.is_absolute() or ".." in path.parts or path.parts[0] != distribution["prefix"]:
                raise ValueError(f"unexpected path in portable CMake archive: {entry.filename}")

            if not entry.is_dir():
                relative = PurePosixPath(*path.parts[1:])
                desired[f"tools/cmake/{relative.as_posix()}"] = entry

        for source in desired.values():
            if isinstance(source, Path) and not source.is_file():
                raise FileNotFoundError(f"SDK input is missing: {source}")

        output_root.mkdir(parents=True, exist_ok=True)
        desired = {f"stakeforge_sdk/{relative}": source for relative, source in desired.items()}

        for path in (source_root / "assets").rglob("*"):
            if path.is_file() and "__pycache__" not in path.parts:
                desired[path.relative_to(source_root / "assets").as_posix()] = path

        desired["packaging/templates/native/.clang-format"] = source_root / ".clang-format"

        copied, removed = synchronize(assets_root, desired, archive)

    for name in ("common", "editor_defaults", "editor_templates", "editor_scaffold", "editor", "stakeforge_sdk"):
        legacy = output_root / name
        validate_destination(output_root, legacy)

        if legacy.is_dir():
            _, deleted = synchronize(legacy, {}, None)
            legacy.rmdir()
            removed += deleted

    for source in runtime_files:
        destination = output_root / source.name
        validate_destination(output_root, destination)

        if source.resolve() != destination.resolve() and (not destination.exists() or not filecmp.cmp(source, destination, shallow=False)):
            writable(destination)
            shutil.copyfile(source, destination)
            copied += 1

    print(f"Stakeforge editor {info.major}.{info.minor}.{info.patch} ({info.build_id}, {settings['configuration']}): "
          f"{copied} copied, {removed} removed; {assets_root}")


def main(argv=None):
    parser = argparse.ArgumentParser(description="Synchronize editor assets, SDK and runtime binaries using CMake build settings.")
    parser.add_argument("--manifest", required=True, type=Path, help="CMake-generated packaging/<configuration>/sdk_build.json")
    args = parser.parse_args(argv)

    try:
        package(args.manifest.resolve())
    except (OSError, ValueError, KeyError, zipfile.BadZipFile) as error:
        print(f"Editor packaging: {error}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
