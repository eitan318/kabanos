#include "vbe.h"
#include "memdefs.h"
#include "memory.h"
#include "s2lib/stdio.h"
#include "stddef.h"
#include "stdint.h"
#include "utils/binary.h"

#define VBE_SUCCESS 0x4f

extern uint32_t bios_vbe_set_mode(uint16_t vbe_mode);
extern uint32_t bios_vbe_info_block_get(void *buffer);
extern uint32_t bios_vbe_get_mode_info(uint16_t mode, void *buffer);

typedef uint16_t vbe_mode_t;
static vbe_info_block_t vbe_controller;

vbe_info_block_t *vbe_init(int *mode_count) {
  uint32_t res = bios_vbe_info_block_get(VBE_TRANSFER_BUFFER);
  if ((res & 0xFF) != VBE_SUCCESS) {
    return NULL;
  }

  memcpy(&vbe_controller, VBE_TRANSFER_BUFFER, sizeof(vbe_info_block_t));

  vbe_mode_t *modes_in_bios = far_ptr_to_phys(vbe_controller.video_mode_ptr);

  int count = 0;
  while (modes_in_bios[count] != 0xFFFF) {
    debugf("mode: %d", modes_in_bios[count]);
    count++;
  }

  *mode_count = count;
  return &vbe_controller;
}

int vbe_set_mode(uint16_t vbe_mode) { return bios_vbe_set_mode(vbe_mode); }
