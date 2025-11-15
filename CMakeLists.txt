# main cmake lists
cmake_minimum_required(VERSION 3.16)
project(myos C ASM_NASM)
# Enable languages
enable_language(C ASM_NASM)

# Force 32-bit build
set(CMAKE_C_FLAGS "-m32 -ffreestanding -fno-pic")
set(CMAKE_EXE_LINKER_FLAGS "-m32 -nostdlib -nodefaultlibs -nostartfiles -no-pie")

# CRITICAL: Set NASM to produce 32-bit objects
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_FLAGS "-f elf32")

find_program(CMAKE_OBJCOPY NAMES objcopy REQUIRED)
find_program(CMAKE_LINKER NAMES ld REQUIRED)
find_package(Python COMPONENTS Interpreter REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set up symlink of compile_commands.json for clang
if(NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
    )
endif()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(OUT_DIR "${CMAKE_BINARY_DIR}/out")
set(BOOTLOADER_OUT_DIR ${OUT_DIR}/bootloader)
set(BOOTLOADER_OUTPUT ${BOOTLOADER_OUT_DIR}/bootloader.bin)
set(KERNEL_OUT_DIR ${OUT_DIR}/kernel)
set(KERNEL_OUTPUT ${KERNEL_OUT_DIR}/kernel.bin)

set(STAGE1_OUT_DIR ${OUT_DIR}/bootloader/stage1)
set(STAGE2_OUT_DIR ${OUT_DIR}/bootloader/stage2)
set(STAGE1_BIN ${STAGE1_OUT_DIR}/stage1.bin)
set(STAGE2_BIN ${STAGE2_OUT_DIR}/stage2.bin)

add_subdirectory("${SRC_DIR}/bootloader")
add_subdirectory("${SRC_DIR}/kernel")

set(OS_IMAGE_OUT ${OUT_DIR}/os.img)
set(BOOT_CONFIG ${CMAKE_SOURCE_DIR}/BOOT/boot.cfg)

set(CREATE_IMAGE_PY "${CMAKE_SOURCE_DIR}/scripts/create_disk_img.py")
add_custom_command(
  OUTPUT ${OS_IMAGE_OUT}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUT_DIR}
  COMMAND ${Python_EXECUTABLE} ${CREATE_IMAGE_PY}
          --image ${OS_IMAGE_OUT}
          --boot ${STAGE1_BIN} 
          --stage2 ${STAGE2_BIN} 
          --kernel ${KERNEL_OUTPUT}
          --boot_cfg ${BOOT_CONFIG}
  DEPENDS ${STAGE2_BIN} ${STAGE1_BIN} ${KERNEL_OUTPUT}
  COMMENT "Creating partitioned OS image (MBR + stage2 + FAT12 partition)"
)

# Targets
add_custom_target(os_image ALL DEPENDS ${OS_IMAGE_OUT})
add_dependencies(os_image kernel bootloader)

# Run target
add_custom_target(run
    COMMAND qemu-system-i386 -debugcon stdio -drive format=raw,file=${OS_IMAGE_OUT}
    DEPENDS os_image
    COMMENT "Running os-image.bin in QEMU"
)


add_custom_target(debug
    COMMAND qemu-system-i386 -debugcon stdio -drive format=raw,file=${OS_IMAGE_OUT} -s -S
    DEPENDS os_image
  COMMENT "Running QEMU with GDB server (port 1234)"
)

#  bochs -f bochsrc -q
