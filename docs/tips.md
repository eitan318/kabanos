│    0x400721        mov    ebp,esp                                                       │
│    0x400723        sub    esp,0x4                                                       │
│    0x400729        mov    eax,0x0                                                       │
│    0x40072e        mov    DWORD PTR [ebp-0x4],eax                                       │
│  > 0x400731        nop                                                                  │
│    0x400732        nop                                                                  │
│    0x400733        nop                                                                  │
│    0x400734        nop                                                                  │
│    0x400735        nop                                                                  │
│    0x400736        nop                                                                  │
│    0x400737        nop                                                                  │
│    0x400738        nop                                                                  │
│    0x400739        mov    eax,0x4134ea                                                  │
│    0x40073e        push   eax                                                           │
│    0x40073f        call   0x40084c                                                      │


when my mkfs script:

(gdb) p* sb
$2 = {on_disk = {magic = 268549871, total_inodes = 1220, total_blocks = 4882, free_inodes = 0, 
free_blocks = 0, block_sectors = 4, file_initial_blocks = 1,
    inode_bitmap_start = 4, block_bitmap_start = 1, inode_table_start = 5, data_blocks_start = 77,
    root_inode = 1}, dev = 0x7fffffffdbd0, block_bytes = 2048,
  block_bitmap = 0x55555555dde0 "\377\377\377\377\377\377\377\377\377?", 
  inode_bitmap = 0x55555555dd30 "\002", inode_hash_table = {0x0, 0x55555555e0d0,
    0x0 <repeats 254 times>}, mounted = 1, plt = 0x55555555c480}
(gdb)

after when opened from disk inside os:

(gdb) p sb->on_disk
$1 = {magic = 268549871, total_inodes = 8189, total_blocks = 32759, free_inodes = 0,
  free_blocks = 0, block_sectors = 4, file_initial_blocks = 1,
  inode_bitmap_start = 17, block_bitmap_start = 1, inode_table_start = 21,
  data_blocks_start = 501, root_inode = 1}
(gdb)

in driver
static vnode_t *myfsd_v_lookup(vnode_t *dir, const char *name) {
  MyfsInode *child_inode = myfs_iget(myfs_sb, child_ino);
