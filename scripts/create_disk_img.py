#!/usr/bin/env python3
import argparse
import os
import math
import subprocess
import tempfile

SECTOR = 512
TOTAL_SECTORS = 4000


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

    # Quick existence check
    for f in [args.boot, args.stage2, args.kernel, args.boot_dir]:
        if not os.path.exists(f):
            raise SystemExit(f"Error: {f} not found")

    # Geometry calculations
    stage2_sectors = math.ceil(os.path.getsize(args.stage2) / SECTOR)
    part_start = 1 + stage2_sectors
    part_sectors = TOTAL_SECTORS - part_start

    if part_start >= TOTAL_SECTORS:
        raise SystemExit("Error: stage2 is too large for image.")

    print(f"Building {args.image}:")
    print(f"  Stage2:    {stage2_sectors} sectors (starts @ 1)")
    print(f"  Partition: {part_sectors} sectors (starts @ {part_start})")

    # Initialize image
    with open(args.image, "wb") as f:
        f.truncate(TOTAL_SECTORS * SECTOR)

    # Write Bootloader & Stage2
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
            "seek=1",
            "conv=notrunc",
        ]
    )

    # Create & Format Partition
    with tempfile.NamedTemporaryFile(delete=False) as tmp:
        part_img = tmp.name

    try:
        with open(part_img, "wb") as f:
            f.truncate(part_sectors * SECTOR)

        run(["mkfs.fat", "-F", "12", part_img])
        run(["mcopy", "-i", part_img, args.kernel, "::kernel.elf"])
        copy_all_boot_files(args.boot_dir, part_img)

        # Write Partition to Image
        run(
            [
                "dd",
                f"if={part_img}",
                f"of={args.image}",
                "bs=512",
                f"seek={part_start}",
                "conv=notrunc",
            ]
        )

        # Update Partition Table
        sfdisk_input = f"{part_start} , {part_sectors} , 1 , *\n"
        subprocess.run(
            ["sfdisk", args.image],
            input=sfdisk_input.encode("utf-8"),
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        print("Success: Image created and partitioned.")
    finally:
        if os.path.exists(part_img):
            os.remove(part_img)


if __name__ == "__main__":
    main()
