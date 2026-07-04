/**
 * @file sys_cwd.c
 * @brief chdir/getcwd syscalls.
 *
 * getcwd reconstructs the path by walking ".." upward and searching each
 * parent directory for the child's inode number, building the string
 * back-to-front.
 */
#include "sys_cwd.h"
#include "fs/fd.h"
#include "fs/vfs_internal.h"
#include "klib/errno.h"
#include "klib/string.h"
#include "ksys/fcntl.h"
#include "ksys/stat.h"
#include "mm/vmspace.h"
#include "proc/proc.h"
#include "sched/dispatcher.h"

#define MAX_PATH 2046

int sys_chdir(const char *path) {
  process_t *curr_proc = dispatch_get_current()->process;
  vnode_t *new_wd = vfs_lookup_path(path, curr_proc->cwd, true);

  if (!new_wd)
    return -ENOENT;
  if (!S_ISDIR(new_wd->mode)) {
    vfs_vnode_put(new_wd);
    return -ENOTDIR;
  }

  // Reference Count Swap
  vnode_t *old_wd = curr_proc->cwd;
  curr_proc->cwd = new_wd;

  if (old_wd) {
    vfs_vnode_put(old_wd);
  }

  return 0;
}

int sys_getcwd(char *buf, size_t size) {
  process_t *proc = dispatch_get_current()->process;

  vnode_t *curr_vnode = proc->cwd;
  vfs_vnode_get_ref(curr_vnode);

  char path_stack[MAX_PATH];
  int path_ptr = MAX_PATH - 1;
  path_stack[path_ptr] = '\0';

  while (1) {
    fstat_t curr_stat;
    int curr_fd = vfs_bind_vnode_to_fd(curr_vnode, O_RDONLY);
    vfs_fstat(curr_fd, &curr_stat);

    // Open ".." relative to our current owned vnode
    int parent_fd = vfs_open("..", curr_vnode, O_RDONLY, 0);
    if (parent_fd < 0) {
      vfs_close(curr_fd);
      vfs_vnode_put(curr_vnode);
      return parent_fd;
    }

    fstat_t parent_stat;
    vfs_fstat(parent_fd, &parent_stat);

    // ROOT CHECK
    if (curr_stat.ino == parent_stat.ino) {
      vfs_close(curr_fd);
      vfs_close(parent_fd);
      break;
    }

    vdir_entry_t entry;
    bool found = false;
    // Search parent for our current inode name
    while (vfs_getdents(parent_fd, &entry, 1) > 0) {
      if (entry.inode_num == curr_stat.ino) {
        size_t len = strlen(entry.file_name);
        if (path_ptr - (int)len - 1 < 0) {
          vfs_close(curr_fd);
          vfs_close(parent_fd);
          vfs_vnode_put(curr_vnode);
          return -ENAMETOOLONG;
        }
        path_ptr -= len;
        memcpy(&path_stack[path_ptr], entry.file_name, len);
        path_stack[--path_ptr] = '/';
        found = true;
        break;
      }
    }
    vnode_t *parent_node = g_fd_table[parent_fd]->vnode;
    vfs_vnode_get_ref(parent_node); // grab our ref while fd still owns one
    vfs_close(parent_fd);           // fd releases its ref; ours keeps it alive
    vfs_close(curr_fd);

    curr_vnode = parent_node;

    if (!found) {
      vfs_vnode_put(curr_vnode);
      return -ENOENT;
    }
  }

  // Release the final reference held by the loop
  vfs_vnode_put(curr_vnode);

  if (path_stack[path_ptr] == '\0') {
    path_stack[--path_ptr] = '/';
  }

  char *final_path = &path_stack[path_ptr];
  size_t final_len = strlen(final_path) + 1;

  if (final_len > size)
    return -ERANGE;

  // Use safe copy to user space
  vmspace_copy_to(proc->vmspace->arch, (vaddr_t)buf, (vaddr_t)final_path,
                  final_len);

  return 0;
}
