/**
 * @file fd.c
 * @brief Global fd table and standard-stream setup.
 *
 * fd 0 = keyboard, 1/2 = console, 3 = debug serial.
 */
#include "fs/fd.h"
#include "ksys/fcntl.h"
#include "mm/kmalloc.h"

/* These vnodes are provided by the respective device drivers */
extern vnode_t kbd_vnode_static;
extern vnode_t con_vnode_static;
extern vnode_t dbg_vnode_static;

file_t *g_fd_table[MAX_FD];

void vfs_init_stdio(void) {
  g_fd_table[0] = kmalloc(sizeof(file_t));
  g_fd_table[0]->vnode = &kbd_vnode_static;
  g_fd_table[0]->f_ops = kbd_vnode_static.super_block->f_ops;
  g_fd_table[0]->flags = O_RDONLY;

  g_fd_table[1] = kmalloc(sizeof(file_t));
  g_fd_table[1]->vnode = &con_vnode_static;
  g_fd_table[1]->f_ops = con_vnode_static.super_block->f_ops;
  g_fd_table[1]->flags = O_WRONLY;

  g_fd_table[2] = kmalloc(sizeof(file_t));
  memcpy(g_fd_table[2], g_fd_table[1], sizeof(file_t));

  g_fd_table[3] = kmalloc(sizeof(file_t));
  g_fd_table[3]->vnode = &dbg_vnode_static;
  g_fd_table[3]->f_ops = dbg_vnode_static.super_block->f_ops;
  g_fd_table[3]->flags = O_WRONLY;
}
