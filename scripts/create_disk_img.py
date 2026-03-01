#!/usr/bin/env python3
import argparse
import os
import math
import subprocess
import tempfile

SECTOR = 512
TOTAL_SECTORS = 2 * 131072  # 64MB


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

    stage2_sectors = math.ceil(os.path.getsize(args.stage2) / SECTOR)

    # Leave some gap after stage2 before partition starts
    p1_start = 1 + stage2_sectors + 1
    p1_sectors = 131072

    p2_start = p1_start + p1_sectors
    p2_sectors = TOTAL_SECTORS - p2_start - 1

    print(f"Building {args.image}:")
    print(f"  Stage2:    sectors 1..{stage2_sectors}")
    print(
        f"  P1 (FAT32): start={p1_start}, size={p1_sectors} ({p1_sectors*SECTOR//1024//1024}MB)"
    )
    print(f"  P2 (myfs):  start={p2_start}, size={p2_sectors} sectors")

    # 1. Initialize empty image
    with open(args.image, "wb") as f:
        f.truncate(TOTAL_SECTORS * SECTOR)

    # 2. Write MBR bootloader (sector 0)
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

    # 3. Write Stage2 (sectors 1..stage2_sectors)
    run(
        [
            "dd",
            f"if={args.stage2}",
            f"of={args.image}",
            "bs=512",
            "seek=1",
            "conv=notrunc",
        ]
    )

    # 4. Create FAT32 partition image
    with tempfile.NamedTemporaryFile(delete=False) as tmp:
        p1_img = tmp.name
    try:
        with open(p1_img, "wb") as f:
            f.truncate(p1_sectors * SECTOR)

        run(["mkfs.vfat", "-s", "1", "-F", "32", p1_img])

        run(["mcopy", "-i", p1_img, args.kernel, "::kernel.elf"])
        copy_all_boot_files(args.boot_dir, p1_img)

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

    # 5. Zero out partition 2
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

    # 6. Write partition table (type 'b' = FAT32, 'L' = Linux/custom)
    sfdisk_input = (
        f"{p1_start}, {p1_sectors}, b, *\n"  # FAT32, bootable
        f"{p2_start}, {p2_sectors}, L, -\n"  # custom fs
    )
    subprocess.run(
        ["sfdisk", args.image], input=sfdisk_input.encode("utf-8"), check=True
    )

    print("Done.")


if __name__ == "__main__":
    main()
