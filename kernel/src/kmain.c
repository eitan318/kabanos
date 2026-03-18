#include "adt/range.h"
#include "boot/bootparams.h"
#include "fs/fd.h"
#include "fs/vfs.h"
#include "hal.h"
#include "kernel_boot_info.h"
#include "klib/stddef.h"
#include "klib/stdio.h"
#include "mm/kmalloc.h"
#include "mm/memdefs.h"
#include "mm/memory_map.h"
#include "mm/pmm.h"
#include "mm/vmspace.h"
#include "modules.h"
#include "panic.h"
#include "proc/sys_exec.h"
#include "sched/sched.h"
#include "sched/thread.h"
#include "ut/frame_allocator_ut_main.h"
#include "ut/fs/fs_ut_main.h"
#include <cmdline.h>

/**
 * @mainpage MyOS Kernel Documentation
 * * @section subsystems Kernel Subsystems
 * - @ref scheduler : Manages CPU time and thread states.
 * - @ref vfs : Virtual File System abstraction.
 * - @ref process_mgr : Process lifecycle and address space management.
 * * @section arch Hardware Abstraction Layer
 * - @ref hal_interface : The mandatory interface for porting to new CPUs.
 */

vmspace_t g_kernel_vmspace_obj;
vmspace_t *g_kernel_vmspace = &g_kernel_vmspace_obj;
range_t g_kernel_virt_range;
range_t g_kernel_phys_range;

static void kmain_init_services(KernelBootInfo *boot_info) {
  vfs_init_stdio();
  modules_init_registry(boot_info->modules);
  modules_load();

  // Mount Root
  char *dev = cmdline_get_arg_copy(boot_info->cmdline, "root_device");
  char *fs = cmdline_get_arg_copy(boot_info->cmdline, "fs_name");

  if (dev && fs) {
    if (vfs_mount(dev, "/", NULL, fs, 0, NULL) < 0) {
      kdebugf("Warning: Could not mount root device %s\n", dev);
    }
  }

  kfree(dev);
  kfree(fs);
}

static void kmain_init_memory(KernelBootInfo *boot_info) {
  range_t total_range = get_memory_range(&boot_info->memory_map);
  size_t used_count, usable_count;

  range_t *used = get_used_memory_ranges(boot_info, total_range, &used_count);
  range_t *usable = get_useable_memory_ranges(boot_info, &usable_count);

  pmm_init(total_range, usable, usable_count, used, used_count);

  kernel_vmspace_create(g_kernel_vmspace, total_range);
  vmspace_switch(g_kernel_vmspace);

  kmalloc_init();
}

static void kmain_launch_init(const char *cmdline) {
  char *init_path = cmdline_get_arg_copy(cmdline, "init_proc");

  // Fallback if cmdline doesn't specify an init
  if (!init_path) {
    panic("Cmdline didnt specify init path");
  }

  if (process_spawn(init_path, 0, NULL, NULL, THREAD_PRIORITY_HIGH) < 0) {
    panic("Faild to spawn process %s", init_path);
  }

  kfree(init_path);
}

void kmain(uint32_t mb2_ptr) {
  kdebugf("[Kernel starting...]\n");

  // Critical Early Setup (Symbols and Multiboot)
  extern uint8_t _kernel_start[], _kernel_end[];
  g_kernel_virt_range.start = (uintptr_t)&_kernel_start;
  g_kernel_virt_range.end = (uintptr_t)&_kernel_end;
  g_kernel_phys_range.start = g_kernel_virt_range.start - KERNEL_BASE;
  g_kernel_phys_range.end = g_kernel_virt_range.end - KERNEL_BASE;

  KernelBootInfo *boot_info = parse_multiboot2_early((mb2_info_t *)mb2_ptr);

  kmain_init_memory(boot_info);
  kmain_init_services(boot_info);
  kmain_launch_init(boot_info->cmdline);

  hal_timer_enable();
  sched_switch_next();

  // Close qemu
  hal_out16(0x604, 0x2000);
}
