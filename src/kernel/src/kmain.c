#include "adt/range.h"
#include "boot/bootparams.h"
#include "device.h"
#include "drivers/block/blockdev.h"
#include "fs/fd.h"
#include "fs/myfs/myfs.h"
#include "fs/vfs.h"
#include "hal.h"
#include "kernel_boot_info.h"
#include "klib/stddef.h"
#include "klib/stdio.h"
#include "ksys/fcntl.h"
#include "mm/kmalloc.h"
#include "mm/memdefs.h"
#include "mm/memory_map.h"
#include "mm/pmm.h"
#include "mm/vmspace.h"
#include "modules.h"
#include "proc/exec.h"
#include "sched/sched.h"
#include "sched/thread.h"
#include "ut/frame_allocator_ut_main.h"
#include "ut/fs/fs_ut_main.h"

vmspace_t g_kernel_vmspace_obj;
vmspace_t *g_kernel_vmspace = &g_kernel_vmspace_obj;
Range g_kernel_virt_range;
Range g_kernel_phys_range;

int ls(const char *path) {
  int fd = vfs_open(path, O_RDONLY);
  if (fd < 0) {
    kprintf("ls: cannot open '%s'\n", path);
    return -1;
  }

  VDirEntry entries[64];
  int count;
  while ((count = vfs_iter_dir(fd, entries, 64)) > 0) {
    for (int i = 0; i < count; i++) {
      fstat_t st;
      char child[512];
      ksnprintf(child, sizeof(child), "%s/%s", path, entries[i].file_name);
      int cfd = vfs_open(child, O_RDONLY);
      if (cfd >= 0 && vfs_fstat(cfd, &st) == 0) {
        char type = '-';
        if (S_ISDIR(st.mode))
          type = 'd';
        if (S_ISLNK(st.mode))
          type = 'l';
        kprintf("%c %llu  %s\n", type, (unsigned long long)st.size,
                entries[i].file_name);
        vfs_close(cfd);
      } else {
        kprintf("?          %s\n", entries[i].file_name);
        if (cfd >= 0)
          vfs_close(cfd);
      }
    }
  }

  vfs_close(fd);
  return 0;
}

void kmain(uint32_t mb2_ptr) {
  kdebugf("[Kernel starting...]\n");

  extern uint8_t _kernel_start[], _kernel_end[];

  g_kernel_virt_range.start = (uintptr_t)&_kernel_start;
  g_kernel_virt_range.end = (uintptr_t)&_kernel_end;
  g_kernel_phys_range.start = g_kernel_virt_range.start - KERNEL_BASE;
  g_kernel_phys_range.end = g_kernel_virt_range.end - KERNEL_BASE;

  KernelBootInfo *kernel_boot_info =
      parse_multiboot2_early((mb2_info_t *)mb2_ptr);
  mb2_ptr = 0; // disabling use of unparsed, low half params

  Range total_memory_range = get_memory_range(&kernel_boot_info->memory_map);
  size_t used_memory_ranges_count;
  Range *used_memory_ranges = get_used_memory_ranges(
      kernel_boot_info, total_memory_range, &used_memory_ranges_count);

  size_t useable_memory_ranges_count;
  Range *useable_memory_ranges =
      get_useable_memory_ranges(kernel_boot_info, &useable_memory_ranges_count);

  // From now on, no early pmm
  pmm_init(total_memory_range, useable_memory_ranges,
           useable_memory_ranges_count, used_memory_ranges,
           used_memory_ranges_count);

  kernel_vmspace_create(g_kernel_vmspace, total_memory_range);
  if (g_kernel_vmspace == NULL) {
    kdebugf("FAIL: Could not create page directory\n");
    return;
  }
  // From now on no lower half mapping
  vmspace_switch(g_kernel_vmspace);

  kmalloc_init();

  vfs_init_stdio();

  // Load static and dynamic modules
  modules_init_registry(kernel_boot_info->modules);
  modules_load();

  blkdev_t *dev = blkdev_get("atap2");
  if (vfs_mount("atap2", "/", "myfs", 0, NULL) < 0) {
    kprintf("couldnt moount!");
  }

  ut_fs_main();

  for (;;) {
  }

  hal_timer_enable();

  sched_yield();

  kprintf("problematic return");

  for (;;) {
  }
}
