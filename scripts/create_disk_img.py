#!/usr/bin/env python3
import argparse
import os
import math
import subprocess
import tempfile

SECTOR = 512
# Increased to ~32MB to fit a "big" partition
TOTAL_SECTORS = 65536


def run(cmd):
    subprocess.run(
        cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )


def copy_all_boot_files(boot_dir, part_img):
    for entry in os.listdir(boot_dir):
        path = os.path.join(boot_dir, entry)
        if os.path.isfile(path):
            run(["mcopy", "-i", part_img, path, f"::{entry}"])


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--image", required=True)
    p.add_argument("--boot", required=True)
    p.add_argument("--stage2", required=True)
    p.add_argument("--kernel", required=True)
    p.add_argument("--boot_dir", required=True)
    args = p.parse_args()

    # Geometry calculations
    stage2_sectors = math.ceil(os.path.getsize(args.stage2) / SECTOR)

    # Partition 1: Boot (FAT12) - Let's give it ~2MB
    p1_start = 1 + stage2_sectors
    p1_sectors = 30000

    # Partition 2: Main Data (myfs) - Remainder of the disk
    p2_start = p1_start + p1_sectors
    p2_sectors = TOTAL_SECTORS - p2_start - 1  # -1 for safety buffer

    print(f"Building {args.image}:")
    print(f"  P1 (Boot): Starts @ {p1_start}, Size: {p1_sectors} sectors")
    print(f"  P2 (Main): Starts @ {p2_start}, Size: {p2_sectors} sectors")

    # 1. Initialize empty image
    with open(args.image, "wb") as f:
        f.truncate(TOTAL_SECTORS * SECTOR)

    # 2. Write MBR and Stage2
    run(
        [
            "dd",
            f"if={args.boot}",
            f"of={args.image}",
            "bs=512",
            "count=1",
            "conv=notrunc",
        ]
    )
    run(
        [
            "dd",
            f"if={args.stage2}",
            f"of={args.image}",
            "bs=512",
            f"seek=1",
            "conv=notrunc",
        ]
    )

    # 3. Create Partition 1 (FAT12)
    with tempfile.NamedTemporaryFile(delete=False) as tmp1:
        p1_img = tmp1.name
    try:
        with open(p1_img, "wb") as f:
            f.truncate(p1_sectors * SECTOR)
        run(["mkfs.fat", "-F", "12", p1_img])
        run(["mcopy", "-i", p1_img, args.kernel, "::kernel.elf"])
        copy_all_boot_files(args.boot_dir, p1_img)

        # Burn P1 into image
        run(
            [
                "dd",
                f"if={p1_img}",
                f"of={args.image}",
                "bs=512",
                f"seek={p1_start}",
                "conv=notrunc",
            ]
        )
    finally:
        if os.path.exists(p1_img):
            os.remove(p1_img)

    # 4. Create Partition 2 (Your custom FS)
    # Note: This assumes you have a utility or 'dd' approach to zero it out
    # If you have a 'mkmyfs' utility, run it here.
    run(
        [
            "dd",
            "if=/dev/zero",
            f"of={args.image}",
            "bs=512",
            f"seek={p2_start}",
            f"count={p2_sectors}",
            "conv=notrunc",
        ]
    )

    # 5. Update Partition Table with sfdisk
    # Format: start, size, type, bootable
    sfdisk_input = (
        f"{p1_start}, {p1_sectors}, L, *\n"  # P1: Bootable
        f"{p2_start}, {p2_sectors}, L, -\n"  # P2: Data
    )
    subprocess.run(
        ["sfdisk", args.image], input=sfdisk_input.encode("utf-8"), check=True
    )

    print("Success: Multi-partition image created.")


if __name__ == "__main__":
    main()
