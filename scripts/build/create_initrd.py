#!/usr/bin/env python3
import os
import tarfile
import sys
import argparse


def make_initrd(src_dir, output_path):
    if not os.path.isdir(src_dir):
        print(f"Error: '{src_dir}' is not a directory")
        return False

    with tarfile.open(output_path, "w", format=tarfile.USTAR_FORMAT) as tar:
        for root, _, files in os.walk(src_dir):
            for f in files:
                full = os.path.join(root, f)
                rel = os.path.relpath(full, src_dir)
                tar.add(full, arcname=rel)

    return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Create initrd image")
    parser.add_argument(
        "--directory", "-d", required=True, help="Directory to pack into the initrd"
    )
    parser.add_argument("--output", "-o", required=True, help="Output initrd file path")

    args = parser.parse_args()

    if make_initrd(args.directory, args.output):
        print(f"Created initrd: {args.output}")
    else:
        sys.exit(1)
