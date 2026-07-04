/**
 * @file kernel_boot_info.h
 * @brief Boot information handed over from the bootloader (Multiboot2).
 */
#pragma once
#include "boot/bootparams.h"
#include "klib/stddef.h"
#include "mm/memory_map.h"
#include "modules.h"

/** @brief Parsed, kernel-friendly view of the Multiboot2 boot information. */
typedef struct {
  char *cmdline;           /**< Kernel command line string. */
  module_t *modules;       /**< Boot modules passed by the bootloader. */
  int module_count;        /**< Number of entries in @ref modules. */
  memory_map_t memory_map; /**< Physical memory map. */
  uint32_t initrd_start;   /**< Physical address of the initrd image. */
  uint32_t initrd_size;    /**< Size of the initrd image in bytes. */
} KernelBootInfo;

/**
 * @brief Parses the raw Multiboot2 info structure early in boot.
 *
 * Must run before the memory managers come up, since it reads
 * bootloader-owned memory that may later be reclaimed.
 */
KernelBootInfo *parse_multiboot2_early(mb2_info_t *mbi);

/**
 * @brief Computes the memory ranges inside @p memory_range that are already
 *        in use (kernel image, modules, initrd, boot info).
 * @param out_count [out] Number of ranges returned.
 * @return Pointer to a static array, overwritten by the next call.
 */
range_t *get_used_memory_ranges(KernelBootInfo *kbi, range_t memory_range,
                                size_t *out_count);

/**
 * @brief Computes the RAM ranges that are free for the physical allocator.
 * @param out_count [out] Number of ranges returned.
 * @return Pointer to a static array, overwritten by the next call.
 */
range_t *get_useable_memory_ranges(KernelBootInfo *kbi, size_t *out_count);
