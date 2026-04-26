from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


def find_clang_format(repo_root: Path) -> Path:
    candidates = [
        repo_root / "utils" / "clang-format.exe",
        repo_root / "utils" / "clang-format",
    ]
    for candidate in candidates:
        if candidate.is_file():
            return candidate

    clang_format = shutil.which("clang-format")
    if clang_format is not None:
        return Path(clang_format)

    raise FileNotFoundError("clang-format was not found")


def collect_files(src_root: Path) -> list[Path]:
    patterns = ("*.h", "*.hpp", "*.c", "*.cpp")
    files = []
    for pattern in patterns:
        files.extend(src_root.rglob(pattern))
    files.sort()
    return files


def collect_source_roots(repo_root: Path) -> list[Path]:
    roots = []
    candidates = [repo_root / "src"]
    candidates.extend(sorted(repo_root.glob("*/src")))

    seen = set()
    for candidate in candidates:
        if not candidate.is_dir():
            continue

        resolved = candidate.resolve()
        if resolved in seen:
            continue

        seen.add(resolved)
        roots.append(resolved)

    return roots


def collect_requested_files(repo_root: Path, paths: list[str]) -> list[Path]:
    files = []
    seen = set()

    for raw_path in paths:
        path = Path(raw_path)
        if not path.is_absolute():
            path = (Path.cwd() / path).resolve()
        else:
            path = path.resolve()

        try:
            path.relative_to(repo_root)
        except ValueError:
            print(f"skipping path outside repository: {raw_path}", file=sys.stderr)
            continue

        if path.is_dir():
            candidates = collect_files(path)
        else:
            candidates = [path]

        for candidate in candidates:
            if not candidate.is_file():
                print(f"skipping missing file: {candidate}", file=sys.stderr)
                continue
            if candidate.suffix not in {".h", ".hpp", ".c", ".cpp"}:
                print(f"skipping unsupported file: {candidate}", file=sys.stderr)
                continue
            if candidate in seen:
                continue
            seen.add(candidate)
            files.append(candidate)

    files.sort()
    return files


def format_files(clang_format: Path, repo_root: Path, files: list[Path], check: bool) -> int:
	if not files:
		return 0

	if check:
		command = [
			str(clang_format),
			"-style=file",
			"--dry-run",
			"--Werror",
		]
		command.extend(str(path) for path in files)
		result = subprocess.run(command, cwd=repo_root)
		return result.returncode

	for path in files:
		command = [
			str(clang_format),
			"-style=file",
			str(path),
		]
		result = subprocess.run(command, cwd=repo_root, stdout=subprocess.PIPE)
		if result.returncode != 0:
			return result.returncode

		path.write_bytes(result.stdout)

	return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.add_argument("paths", nargs="*")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    style_file = repo_root / ".clang-format"

    if not style_file.is_file():
        print(f"missing style file: {style_file}", file=sys.stderr)
        return 1

    try:
        clang_format = find_clang_format(repo_root)
    except FileNotFoundError as error:
        print(str(error), file=sys.stderr)
        return 1

    if args.paths:
        files = collect_requested_files(repo_root, args.paths)
    else:
        src_roots = collect_source_roots(repo_root)
        if not src_roots:
            print(f"missing source directories under: {repo_root}", file=sys.stderr)
            return 1

        files = []
        for src_root in src_roots:
            files.extend(collect_files(src_root))

    return format_files(clang_format, repo_root, files, args.check)


if __name__ == "__main__":
    raise SystemExit(main())
