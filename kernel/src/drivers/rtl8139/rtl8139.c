#include "drivers/rtl8139/rtl8139.h"
#include "hal.h"
#include "isr.h"
#include "utils/math.h"
#include "arch/i686/pic.h"
#include "mm/kmalloc.h"
#include "mm/pmm.h"
#include "mm/memdefs.h"
#include "drivers/pci/pci.h"
#include "net/net_syscalls.h"
#include "klib/string.h"
#include "klib/stdio.h"

#define RTL8139_BASE_ADDR 0xC000
#define RTL8139_IRQ 11
#define RTL8139_INT 0x2b
#define RTL8139_RX_BUFFER_SIZE (8192 + 16)
#define RX_COPY_BUFFER_SIZE 2048
#define RTL8139_MAX_TRANSMIT_SIZE 1792

/* Essential register offsets */
// not all registers - for minimal driver at start
#define RTL_CONFIG_1 0x52  // Configuration register 1
#define RTL_CR      0x37   // Command register
#define RTL_CAPR    0x38   // CAPR register - Current Address of Packet Read
#define RTL_ISR     0x3E   // Interrupt status
#define RTL_IMR     0x3C   // Interrupt mask
#define RTL_TSD0    0x10   // TX descriptor 0 status
#define RTL_TSAD0   0x20   // TX descriptor 0 start address
#define RTL_RBSTART 0x30   // RX buffer start address
#define RTL_TCR     0x40   // TX configuration register
#define RTL_RCR     0x44   // RX configuration

/* Command register flags */
// by enabling it we able to Receive and Transmit
#define CR_RST 0x10  // Reset
#define CR_RE  0x08  // Receiver enable
#define CR_TE  0x04  // Transmit enable

/* Interrupt bits */
// by enabling it we tell to the NIC to trigger an interrupt only at TOK and ROK events
#define IMR_ROK 0x01  // Receive OK interrupt enable
#define IMR_TOK 0x04  // Transmit OK interrupt enable

/* ISR bits */
// we will use that to check if the interrupt had happened because ROK or TOK
#define ISR_ROK 0x01  // Receive OK
#define ISR_TOK 0x04  // Transmit OK

// Loopback mode bits for TCR (Transmit Configuration Register)
#define TCR_LBK_MODE1   0x00060000  // Loopback mode 1 (internal)

static void *rtl8139_rx_buffer_phys;  // Physical address for NIC
static void *rtl8139_rx_buffer_virt;  // Virtual address for CPU
static void *rtl8139_tx_buffer_phys;  // Physical address for NIC
static void *rtl8139_tx_buffer_virt;  // Virtual address for CPU

// Network Configuration
static uint8_t rtl8139_mac[6];
static uint8_t my_ip_address[4] = {10, 0, 0, 100};      // Our OS IP: 10.0.0.100
static uint8_t subnet_mask[4] = {255, 255, 255, 0};     // Subnet: 255.255.255.0
static uint8_t gateway_ip[4] = {10, 0, 0, 1};           // Gateway: 10.0.0.1

static uint8_t received_packet_buffer[RX_COPY_BUFFER_SIZE];
static uint32_t received_packet_length = 0;
static bool packet_ready = false;
static uint8_t current_tx_desc = 0;

static uint16_t current_offset = 0;

extern vmspace_t *g_kernel_vmspace;

static void* pmm_alloc_contiguous_frames(uint32_t frame_count) {
    uint64_t first = pmm_frame_alloc();
    if (!first) return NULL;

    for (uint32_t i = 1; i < frame_count; i++) {
        uint64_t next = pmm_frame_alloc();
        if (next != first + i * FRAME_SIZE) {
            kdebugf("PMM returned non-contiguous frames!\n");
            for (;;);
        }
    }

    return (void*)(uintptr_t)first;
}

static void rtl8139_isr_handler(trap_frame_t *r) {
  uint16_t status = hal_in16(RTL8139_BASE_ADDR + RTL_ISR);

  // Writing 1 back to the ISR to clears it. 
  hal_out16(RTL8139_BASE_ADDR + RTL_ISR, ISR_ROK | ISR_TOK); 
  if(status & ISR_TOK) {
	  // Sent
	  // have nothing to do because we already clear TOK in two lines above
  }
  if (status & ISR_ROK) {	
      // Receive packet into global buffer
      received_packet_length = rtl8139_receive_packet(received_packet_buffer, sizeof(received_packet_buffer));

      if (received_packet_length > 0) {
          packet_ready = true;
          net_socket_deliver(received_packet_buffer, received_packet_length);
      }
  }
  
eoi:
  i686_pic_send_eoi(RTL8139_IRQ);
}

void rtl8139_init(module_t *module) {
    rtl8139_enable_bus_mastering(0, 3); 
    // set the LWAKE + LWPTN to active high. this should essentially *power on* the device.
    hal_out8(RTL8139_BASE_ADDR + RTL_CONFIG_1, 0x0); 
    
    rtl8139_reset();
    
    uint32_t rx_frames = (RTL8139_RX_BUFFER_SIZE + FRAME_SIZE - 1) / FRAME_SIZE;
    rtl8139_rx_buffer_phys = pmm_alloc_contiguous_frames(rx_frames);
    if (!rtl8139_rx_buffer_phys) {
        kdebugf("RTL8139: failed to allocate DMA buffers\n");
        for (;;);
    }

	// Map RX buffer to virtual address space
    rtl8139_rx_buffer_virt = (void*)((uintptr_t)rtl8139_rx_buffer_phys + KERNEL_BASE);
    if (!hal_vm_map_range(g_kernel_vmspace->arch, 
                      (paddr_t)(uintptr_t)rtl8139_rx_buffer_phys,
                      (vaddr_t)(uintptr_t)rtl8139_rx_buffer_virt,
                      align_up(RTL8139_RX_BUFFER_SIZE, PAGE_SIZE),
                      PAGE_READWRITE)) {
        kdebugf("RTL8139: failed to map RX buffer\n");
        for (;;);
    }
    
    // Give NIC the physical address
    hal_out32(RTL8139_BASE_ADDR + RTL_RBSTART, (uint32_t)(uintptr_t)rtl8139_rx_buffer_phys);
    
    // Allocate TX buffer from PMM (like RX buffer) so we have physical address
    rtl8139_tx_buffer_phys = pmm_alloc_contiguous_frames(1);  // 1 frame is enough for max packet
    if (!rtl8139_tx_buffer_phys) {
        kdebugf("RTL8139: failed to allocate TX buffer\n");
        for (;;);
    }
 
	// Map TX buffer to virtual address space
    rtl8139_tx_buffer_virt = (void*)((uintptr_t)rtl8139_tx_buffer_phys + KERNEL_BASE);
    if (!hal_vm_map_range(g_kernel_vmspace->arch,
                      (paddr_t)(uintptr_t)rtl8139_tx_buffer_phys,
                      (vaddr_t)(uintptr_t)rtl8139_tx_buffer_virt,
                      FRAME_SIZE,
                      PAGE_READWRITE)) {
        kdebugf("RTL8139: failed to map TX buffer\n");
        for (;;);
    }
	
    kdebugf("RTL8139: TX buffer phys=0x%x, virt=0x%x\n", 
           (uint32_t)(uintptr_t)rtl8139_tx_buffer_phys,
           (uint32_t)(uintptr_t)rtl8139_tx_buffer_virt);
    
    // Set the RTL8139 to accept only the Transmit OK (TOK) and Receive OK (ROK) interrupts
    // That way when a TOK or ROK IRQ happens, it actually will go through and fire up an IRQ.
    hal_out16(RTL8139_BASE_ADDR + RTL_IMR, IMR_ROK | IMR_TOK); 
    
    // Initialize CAPR before enabling RE/TE
    current_offset = 0;
    hal_out16(RTL8139_BASE_ADDR + RTL_CAPR, 0xFFF0);
    
    // Sets the RE and TE bits high
    // by that the card will start allowing packets in and/or out.
    hal_out8(RTL8139_BASE_ADDR + RTL_CR, CR_RE | CR_TE); 
    
    // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP
    // by that we enabling all packets to recive and wrap = 1
    hal_out32(RTL8139_BASE_ADDR + RTL_RCR, 0xe | (1 << 7)); 

    // Read MAC address from registers 0x00..0x05
	kdebugf("RTL8139 MAC Address: ");
	for (int i = 0; i < 6; i++) {
		rtl8139_mac[i] = hal_in8(RTL8139_BASE_ADDR + i);
	}
	print_mac(rtl8139_mac);
    kdebugf("\n");
	
    // Register rtl8139 interrupt handler
    isr_handler_register(RTL8139_INT, rtl8139_isr_handler);

    // Enable rtl8139 interrupt (IRQ 11)
    i686_pic_unmask_irq(RTL8139_IRQ);
    
    kdebugf("RTL8139: RX buffer phys=0x%x, virt=0x%x\n\n", 
           (uint32_t)(uintptr_t)rtl8139_rx_buffer_phys,
           (uint32_t)(uintptr_t)rtl8139_rx_buffer_virt);
}

void rtl8139_disable() {
    // Disable interrupts
    hal_out16(RTL8139_BASE_ADDR + RTL_IMR, 0);
    
    // Mask the IRQ at the PIC
    i686_pic_mask_irq(RTL8139_IRQ);
    
    // Unregister interrupt handler
    isr_handler_register(RTL8139_INT, NULL);
    
    // Reset the device (stops TX/RX, clears buffers, resets state)
    rtl8139_reset();
    
    // Clear driver state
    packet_ready = false;
    received_packet_length = 0;
    current_offset = 0;
    
    // Unmap and free RX buffer
    if (rtl8139_rx_buffer_virt) {
        hal_vm_unmap_range(g_kernel_vmspace->arch, 
                      (vaddr_t)(uintptr_t)rtl8139_rx_buffer_virt,
                      RTL8139_RX_BUFFER_SIZE);
        
        uint32_t rx_frames = (RTL8139_RX_BUFFER_SIZE + FRAME_SIZE - 1) / FRAME_SIZE;
        for (uint32_t i = 0; i < rx_frames; i++) {
            pmm_frame_free((paddr_t)(uintptr_t)rtl8139_rx_buffer_phys + i * FRAME_SIZE);
        }
        
        rtl8139_rx_buffer_phys = NULL;
        rtl8139_rx_buffer_virt = NULL;
    }
    
    // Unmap and free TX buffer
    if (rtl8139_tx_buffer_virt) {
        hal_vm_unmap_range(g_kernel_vmspace->arch,
                      (vaddr_t)(uintptr_t)rtl8139_tx_buffer_virt,
                      FRAME_SIZE);
        
        pmm_frame_free((paddr_t)(uintptr_t)rtl8139_tx_buffer_phys);
        
        rtl8139_tx_buffer_phys = NULL;
        rtl8139_tx_buffer_virt = NULL;
    }
    
    kdebugf("RTL8139: Device disabled and cleaned up\n");
}

void rtl8139_reset() {
  // when doing a software reset the RST bit must be checked to make sure that the chip has finished the reset. 
  // If the RST bit is high (1), then the reset is still in operation.
  hal_out8(RTL8139_BASE_ADDR + RTL_CR, CR_RST);
  while((hal_in8(RTL8139_BASE_ADDR + RTL_CR) & CR_RST) != 0) { }
}

void rtl8139_send_packet(uint8_t *data, uint32_t length) {
    if (length == 0 || length > RTL8139_MAX_TRANSMIT_SIZE) {
        kdebugf("Error in length of packet\n");
        return;
    }
    //kdebugf("rtl8139_send_packet: length=%d\n", length);
    
	// Use descriptors 0,1,2,3 in rotation - round robin
    uint16_t tsad_reg = RTL_TSAD0 + (current_tx_desc * 4);
    uint16_t tsd_reg = RTL_TSD0 + (current_tx_desc * 4);
	
    memcpy(rtl8139_tx_buffer_virt, data, length);
    hal_out32(RTL8139_BASE_ADDR + tsad_reg, (uintptr_t)rtl8139_tx_buffer_phys);
    hal_out32(RTL8139_BASE_ADDR + tsd_reg, length);
	
	current_tx_desc = (current_tx_desc + 1) % 4;
}

uint32_t rtl8139_receive_packet(uint8_t *buffer, uint32_t buffer_len) {
    // Use VIRTUAL address to access the buffer
    uint8_t *rx_buf = (uint8_t *)rtl8139_rx_buffer_virt;
    
    // Read packet header
    uint16_t *header = (uint16_t *)(rx_buf + current_offset);
    uint16_t status = header[0];
    uint16_t length = header[1];  // This includes 4-byte CRC
    
    // Validate length (must be at least 4 for CRC)
    if (length < 4) {
        kdebugf("RX: Invalid length %u\n", length);
        return 0;
    }
    
    // Actual data length (excluding CRC)
    uint16_t data_len = length - 4;
    if (data_len > buffer_len) {
        kdebugf("RX: Data too large (%u > %u)\n", data_len, buffer_len);
        return 0;
    }
    
    // Copy packet data (skip 4-byte header, exclude CRC at end)
    for (uint32_t i = 0; i < data_len; i++) {
        buffer[i] = rx_buf[(current_offset + 4 + i) % RTL8139_RX_BUFFER_SIZE];
    }
    
    // Calculate next offset with DWORD alignment
    // Total packet size = header(4) + data + CRC(4)
    uint32_t packet_size = 4 + length;  // header + (data + CRC)
    packet_size = (packet_size + 3) & ~3;  // Round up to 4-byte boundary
    
    current_offset = (current_offset + packet_size) % RTL8139_RX_BUFFER_SIZE;
    
    hal_out16(RTL8139_BASE_ADDR + RTL_CAPR, ((uint16_t)(current_offset - 0x10)) % RTL8139_RX_BUFFER_SIZE);

    return data_len;
}

uint32_t rtl8139_get_packet(uint8_t *buffer, uint32_t max_len) {
    uint32_t len = received_packet_length;
    if (len > max_len) {
		len = max_len;
	}
    memcpy(buffer, received_packet_buffer, len);
    return len;
}

void rtl8139_enable_bus_mastering(uint8_t bus, uint8_t slot) {
    uint32_t cmd = pci_config_read32(bus, slot, 0, 0x04);
    cmd |= (1 << 2);
    cmd |= (1 << 0);
    pci_config_write32(bus, slot, 0, 0x04, cmd);
}

void rtl8139_set_loopback(bool enable) {
    uint32_t tcr = hal_in32(RTL8139_BASE_ADDR + RTL_TCR);
    
    if (enable) {
        // Clear existing loopback and IFG bits
        tcr &= ~0x03FE0000;  // Clear bits 17-25 (loopback + IFG)
        
        // Try mode 1 (normal loopback) 
        tcr |= TCR_LBK_MODE1;
        
        // Set InterFrameGap to normal (0x03 << 24)
        tcr |= (0x03 << 24);
        
        kdebugf("RTL8139: Enabling loopback mode 1\n");
    } else {
        tcr &= ~0x03FE0000;
        tcr |= (0x03 << 24);  // Restore normal IFG
        kdebugf("RTL8139: Disabling loopback mode\n");
    }
    
    hal_out32(RTL8139_BASE_ADDR + RTL_TCR, tcr);
}

bool rtl8139_has_packet() {
    return packet_ready;
}

void rtl8139_clear_packet() {
    packet_ready = false;
    received_packet_length = 0;
}

void rtl8139_packet_print(uint8_t *data, uint32_t len) {
	kdebugf("Packet (%u bytes):\n", len);
    
    for (uint32_t i = 0; i < len && i < 256; i += 16) {
        // Print offset with leading zeros
        kdebugf("\n");
        if (i < 0x10) kdebugf("0");
        if (i < 0x100) kdebugf("0");
        if (i < 0x1000) kdebugf("0");
        kdebugf("%x: ", i);
        
        // Print hex bytes
        for (uint32_t j = 0; j < 16; j++) {
            if (i + j < len) {
                if (data[i + j] < 0x10) kdebugf("0");
                kdebugf("%x ", data[i + j]);
            } else {
                kdebugf("   ");  // padding for incomplete lines
            }
        }
        
        // Print ASCII representation
        kdebugf(" | ");
        for (uint32_t j = 0; j < 16 && (i + j) < len; j++) {
            uint8_t c = data[i + j];
            // Print printable ASCII (0x20-0x7E), otherwise print '.'
            if (c >= 0x20 && c <= 0x7E) {
                kdebugf("%c", c);
            } else {
                kdebugf(".");
            }
        }
    }
    kdebugf("\n");
}

void print_mac(uint8_t *mac) {
    for (int i = 0; i < 6; i++) {
        if (mac[i] < 0x10) kdebugf("0");
        kdebugf("%x", mac[i]);
        if (i < 5) kdebugf(":");
    }
}

void rtl8139_print_network_config() {
    kdebugf("\n========================================\n");
    kdebugf("RTL8139 Network Configuration\n");
    kdebugf("========================================\n");
    
    kdebugf("MAC Address:  ");
    print_mac(rtl8139_mac);
    kdebugf("\n");
    
    kdebugf("IP Address:   %d.%d.%d.%d\n", 
           my_ip_address[0], my_ip_address[1], 
           my_ip_address[2], my_ip_address[3]);
    
    kdebugf("Subnet Mask:  %d.%d.%d.%d\n",
           subnet_mask[0], subnet_mask[1], 
           subnet_mask[2], subnet_mask[3]);
    
    kdebugf("Gateway:      %d.%d.%d.%d\n",
           gateway_ip[0], gateway_ip[1], 
           gateway_ip[2], gateway_ip[3]);
    
    kdebugf("========================================\n\n");
}

uint8_t* rtl8139_get_ip() {
    return my_ip_address;
}

uint8_t* rtl8139_get_mac() {
    return rtl8139_mac;
}

uint8_t* rtl8139_get_gateway() {
    return gateway_ip;
}

static const char *rtl8139_deps[] = {"hal", "devices", NULL};

ITER_MODULE(rtl8139) = {
    .name = "rtl8139",
    .required_modules_names = rtl8139_deps,
    .init = &rtl8139_init,
    .fini = NULL,
};
