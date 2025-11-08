#!/usr/bin/env python3
# tools/create_image.py
# Usage:
#   python3 create_image.py --image OUT/os.img \
#       --boot BOOT/bootloader.bin --stage2 BOOT/stage2.bin --kernel KERNEL/kernel.bin

import argparse
import os
import math
import subprocess
import sys
import tempfile

SECTOR = 512
TOTAL_SECTORS = 2880  # 1.44MB floppy-style image


def run(cmd, check=True):
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=check)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--image", required=True)
    p.add_argument("--boot", required=True)  # bootloader (512-byte MBR expected)
    p.add_argument("--stage2", required=True)  # stage2 binary (arbitrary size)
    p.add_argument("--kernel", required=True)  # kernel to put in FAT12 partition
    args = p.parse_args()

    image = args.image
    boot = args.boot
    stage2 = args.stage2
    kernel = args.kernel

    # sanity
    if not os.path.exists(boot):
        raise SystemExit("boot not found")
    if not os.path.exists(stage2):
        raise SystemExit("stage2 not found")
    if not os.path.exists(kernel):
        raise SystemExit("kernel not found")

    stage2_size = os.path.getsize(stage2)
    stage2_sectors = math.ceil(stage2_size / SECTOR)
    print("stage2 size:", stage2_size, "bytes ->", stage2_sectors, "sectors")

    # partition starts AFTER sector 0 (MBR) and stage2 sectors
    part_start = 1 + stage2_sectors
    if part_start >= TOTAL_SECTORS:
        raise SystemExit("stage2 too large to fit on image")

    part_sectors = TOTAL_SECTORS - part_start
    print(
        "partition will start at sector",
        part_start,
        "and length",
        part_sectors,
        "sectors",
    )

    # create blank image
    print("creating blank image:", image)
    with open(image, "wb") as f:
        f.truncate(TOTAL_SECTORS * SECTOR)

    # write bootloader (MBR) into sector 0 (not truncating)
    print("writing bootloader (MBR) to sector 0")
    run(
        [
            "dd",
            "if={}".format(boot),
            "of={}".format(image),
            "bs=512",
            "count=1",
            "conv=notrunc",
        ]
    )

    # write stage2 at offset sector 1
    print("writing stage2 to sector 1..")
    run(
        [
            "dd",
            "if={}".format(stage2),
            "of={}".format(image),
            "bs=512",
            "seek=1",
            "conv=notrunc",
        ]
    )

    # create a temporary file for the partition (the raw partition contents)
    with tempfile.NamedTemporaryFile(delete=False) as tmp:
        part_img = tmp.name
    print("creating temporary partition image:", part_img)
    with open(part_img, "wb") as f:
        f.truncate(part_sectors * SECTOR)

    # Format the partition image as FAT12 (requires mkfs.fat - part of dosfstools)
    print("mkfs.fat -F 12 on partition image (requires mkfs.fat/dosfstools).")
    run(["mkfs.fat", "-F", "12", part_img])

    # Use mtools (mcopy) to copy kernel into the root of the FAT12 image.
    # mtools uses environment variable MTOOLS_SKIP_CHECK=1 sometimes needed
    print("copying kernel into partition image (requires mtools 'mcopy').")
    # ensure mtools can operate on the file: use -i device for mtools to use image directly
    run(["mcopy", "-i", part_img, kernel, "::kernel.bin"])

    # write partition image into final image at partition start
    print("writing partition image into final image at sector", part_start)
    run(
        [
            "dd",
            "if={}".format(part_img),
            "of={}".format(image),
            "bs=512",
            "seek={}".format(part_start),
            "conv=notrunc",
        ]
    )

    # create partition table with sfdisk: type 0x01 is FAT12
    # sfdisk input format: start,size,type,bootable
    print("writing partition table with sfdisk (type 0x01 = FAT12)")
    sfdisk_input = "{} , {} , 1 , *\n".format(part_start, part_sectors)
    # We feed sfdisk through stdin
    proc = subprocess.run(["sfdisk", image], input=sfdisk_input.encode("utf-8"))
    if proc.returncode != 0:
        raise SystemExit("sfdisk failed, check tool installed and permissions")

    # cleanup
    os.remove(part_img)
    print("done. created", image)
    print("partition starts at sector", part_start, "contains kernel as /kernel.bin")


if __name__ == "__main__":
    main()
