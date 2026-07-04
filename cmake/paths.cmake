set(SRC_DIR "${CMAKE_SOURCE_DIR}")
set(OUT_DIR "${CMAKE_BINARY_DIR}")

set(BOOTLOADER_OUT_DIR ${OUT_DIR}/bootloader)
set(BOOTLOADER_OUTPUT ${BOOTLOADER_OUT_DIR}/bootloader.bin)

set(BOOT_DIR ${CMAKE_BINARY_DIR}/bootroot)
set(INITRD_IMG ${BOOT_DIR}/initrd.tar)

set(COMMON_DIR ${SRC_DIR}/common)

set(KERNEL_OUT_DIR ${BOOT_DIR})
set(KERNEL_OUTPUT ${KERNEL_OUT_DIR}/kernel.elf)

set(STAGE1_OUT_DIR ${OUT_DIR}/bootloader/stage1)
set(STAGE2_OUT_DIR ${OUT_DIR}/bootloader/stage2)

set(STAGE1_BIN ${STAGE1_OUT_DIR}/stage1.bin)
set(STAGE2_BIN ${STAGE2_OUT_DIR}/stage2.bin)

set(OS_IMAGE_OUT ${OUT_DIR}/os.img)

set(SYSROOT_DIR ${CMAKE_BINARY_DIR}/sysroot)

set(MYOS_STUBS_SRC "${CMAKE_SOURCE_DIR}/extern/newlib_myos")
set(NEWLIB_SRC_DIR "/src/newlib-2.5.0")
set(NEWLIB_BUILD_DIR "${CMAKE_BINARY_DIR}/build-newlib")
set(NEWLIB_SYSROOT "${SYSROOT_DIR}")



set(CREATE_IMAGE_PY "${CMAKE_SOURCE_DIR}/scripts/build/create_disk_img.py")
set(MAKE_INITRD_PY "${CMAKE_SOURCE_DIR}/scripts/build/create_initrd.py")
set(CALC_STAGE2_SECTORS_SH "${CMAKE_SOURCE_DIR}/scripts/build/calc_stage2_size.sh")
