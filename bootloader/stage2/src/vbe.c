#include "stdint.h"
typedef struct {
  char vbe_signature[4];      // == "VESA"
  uint16_t vbe_version;       // == 0x0300 for VBE 3.0
  uint16_t oem_string_ptr[2]; // isa vbeFarPtr
  uint8_t capabilities[4];
  uint16_t video_mode_ptr[2]; // isa vbeFarPtr
  uint16_t total_memory;      // as # of 64KB blocks
  uint8_t reserved[492];
} __attribute__((packed)) vbe_info_block;
