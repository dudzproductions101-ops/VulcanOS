#!/usr/bin/env python3
import struct
import sys
from pathlib import Path

VPK_MAGIC = b"VULCPKG\0"
VPK_FORMAT_VERSION = 1


def collect_files(package_dir: Path) -> list[Path]:
    return sorted(p for p in package_dir.rglob("*") if p.is_file())


def build_vpk(package_dir: Path, output_path: Path) -> None:
    manifest_path = package_dir / "manifest.vconf"
    if not manifest_path.exists():
        sys.exit(f"error: {package_dir} has no manifest.vconf")

    files = collect_files(package_dir)
    files.remove(manifest_path)
    files.insert(0, manifest_path)

    if len(files) > 64:
        sys.exit(
            f"error: {len(files)} files exceeds VPK_MAX_ENTRIES (64) -- "
            "either trim the package or raise VPK_MAX_ENTRIES in "
            "kernel/include/pkg/vpk_archive.h and rebuild the kernel"
        )

    with open(output_path, "wb") as out:
        out.write(VPK_MAGIC)
        out.write(struct.pack("<I", VPK_FORMAT_VERSION))
        out.write(struct.pack("<I", len(files)))

        for file_path in files:
            rel_path = file_path.relative_to(package_dir).as_posix()
            rel_path_bytes = rel_path.encode("utf-8")

            if len(rel_path_bytes) >= 128:
                sys.exit(
                    f"error: path '{rel_path}' is too long "
                    "(VPK_MAX_PATH is 128 bytes)"
                )

            content = file_path.read_bytes()

            out.write(struct.pack("<H", len(rel_path_bytes)))
            out.write(rel_path_bytes)
            out.write(struct.pack("<Q", len(content)))
            out.write(content)

    print(f"built {output_path} from {len(files)} file(s) in {package_dir}")


def main() -> None:
    if len(sys.argv) != 3:
        sys.exit(__doc__)

    package_dir = Path(sys.argv[1])
    output_path = Path(sys.argv[2])

    if not package_dir.is_dir():
        sys.exit(f"error: {package_dir} is not a directory")

    build_vpk(package_dir, output_path)


if __name__ == "__main__":
    main()