#include "vbe.h"
#include "stddef.h"
#include "stdint.h"

#define MAX_VBE 1024
vbe_info_block_t vbe_info_block_list[MAX_VBE];

extern uint32_t bios_vbe_set_mode(uint16_t vbe_mode);
extern uint32_t bios_vbe_info_block_get(void *buffer);

vbe_info_block_t *vbe_info_get(int *count) {
  uint32_t res = bios_vbe_info_block_get(vbe_info_block_list);
  if (res != 0) {
    return NULL;
  }

  *count = 6;
  return vbe_info_block_list;
}

int vbe_set_mode(uint16_t vbe_mode) { return bios_vbe_set_mode(vbe_mode); }
