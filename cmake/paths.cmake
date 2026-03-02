set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(OUT_DIR "${CMAKE_BINARY_DIR}/out")

set(BOOTLOADER_OUT_DIR ${OUT_DIR}/bootloader)
set(BOOTLOADER_OUTPUT ${BOOTLOADER_OUT_DIR}/bootloader.bin)

set(BOOT_DIR "${CMAKE_BINARY_DIR}/bootroot")
set(INITRD_IMG ${BOOT_DIR}/initrd.tar)

set(KERNEL_OUT_DIR ${BOOT_DIR})
set(KERNEL_OUTPUT ${KERNEL_OUT_DIR}/kernel.elf)

set(STAGE1_OUT_DIR ${OUT_DIR}/bootloader/stage1)
set(STAGE2_OUT_DIR ${OUT_DIR}/bootloader/stage2)

set(STAGE1_BIN ${STAGE1_OUT_DIR}/stage1.bin)
set(STAGE2_BIN ${STAGE2_OUT_DIR}/stage2.bin)

set(OS_IMAGE_OUT ${OUT_DIR}/os.img)

set(SYSROOT_DIR ${CMAKE_BINARY_DIR}/sysroot)

set(CREATE_IMAGE_PY "${CMAKE_SOURCE_DIR}/scripts/create_disk_img.py")
set(MAKE_INITRD_PY "${CMAKE_SOURCE_DIR}/scripts/create_initrd.py")
