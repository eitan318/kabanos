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
set(KERNEL_OUTPUT ${KERNEL_OUT_DIR}/kernel.elf)  

set(STAGE1_OUT_DIR ${OUT_DIR}/bootloader/stage1)
set(STAGE2_OUT_DIR ${OUT_DIR}/bootloader/stage2)
set(STAGE1_BIN ${STAGE1_OUT_DIR}/stage1.bin)
set(STAGE2_BIN ${STAGE2_OUT_DIR}/stage2.bin)

add_subdirectory("${SRC_DIR}/bootloader")
add_subdirectory("${SRC_DIR}/kernel")

set(OS_IMAGE_OUT ${OUT_DIR}/os.img)
set(BOOT_DIR ${CMAKE_SOURCE_DIR}/BOOT)
set(INITRD_DIR ${CMAKE_SOURCE_DIR}/initrd_files)
set(INITRD_IMG ${BOOT_DIR}/initrd.tar)

set(CREATE_IMAGE_PY "${CMAKE_SOURCE_DIR}/scripts/create_disk_img.py")
set(MAKE_INITRD_PY "${CMAKE_SOURCE_DIR}/scripts/create_initrd.py")

# Collect all files in initrd_files directory to use as dependencies
file(GLOB_RECURSE INITRD_FILES "${INITRD_DIR}/*")

# Custom command to create initrd.img from initrd_files directory

add_custom_command(
  OUTPUT ${INITRD_IMG}
  COMMAND ${Python_EXECUTABLE} ${MAKE_INITRD_PY}
          --output ${INITRD_IMG}
          --directory ${INITRD_DIR}
  DEPENDS ${INITRD_FILES} ${MAKE_INITRD_PY}
  COMMENT "Creating initrd.img from ${INITRD_DIR}"
  VERBATIM
)

# Custom command to create the final OS image
add_custom_command(
  OUTPUT ${OS_IMAGE_OUT}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OUT_DIR}
  COMMAND ${Python_EXECUTABLE} ${CREATE_IMAGE_PY}
          --image ${OS_IMAGE_OUT}
          --boot ${STAGE1_BIN} 
          --stage2 ${STAGE2_BIN} 
          --kernel ${KERNEL_OUTPUT}  
          --boot_dir ${BOOT_DIR}
  DEPENDS ${STAGE2_BIN} ${STAGE1_BIN} ${KERNEL_OUTPUT} ${INITRD_IMG}
  COMMENT "Creating partitioned OS image (MBR + stage2 + FAT12 partition)"
)

# Single real target
add_custom_target(os_image
    DEPENDS ${OS_IMAGE_OUT}
)
add_dependencies(os_image bootloader kernel initrd)

# Run target
add_custom_target(run
    COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/run.py
            --kernel ${KERNEL_OUTPUT}
            --image ${OS_IMAGE_OUT}
    DEPENDS ${OS_IMAGE_OUT} os_image
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running OS in QEMU and resolving panic addresses"
    VERBATIM
)
add_custom_target(debug
    COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/run.py
            --kernel ${KERNEL_OUTPUT}
            --image ${OS_IMAGE_OUT}
            --is_debug
    DEPENDS ${OS_IMAGE_OUT} os_image
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
   COMMENT "Running QEMU with GDB server (port 1234)"
    VERBATIM
)

#  bochs -f bochsrc -q
