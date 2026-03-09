# Target System Information
set(CMAKE_SYSTEM_NAME Generic) # "Generic" means there is no OS (bare metal/custom)
set(CMAKE_SYSTEM_PROCESSOR i686)

# Force the 32-bit Cross-Compiler
# Replace these with the actual names of your cross-tools if they have prefixes
set(CMAKE_C_COMPILER i686-myos-gcc)
set(CMAKE_ASM_NASM_COMPILER nasm)

set(CMAKE_SYSROOT "${SYSROOT_DIR}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Global Compilation Flags
set(CMAKE_C_FLAGS "-m32 -ffreestanding -fno-pic" CACHE STRING "")
set(CMAKE_ASM_NASM_FLAGS "-f elf32 -g -F dwarf" CACHE STRING "")

# Global Linker Flags
set(CMAKE_EXE_LINKER_FLAGS "-m32 -nostdlib -static" CACHE STRING "")


