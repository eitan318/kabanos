#include "isr.h"
#include "arch/i686/idt.h"
#include "arch/i686/panic.h"
#include "include/stdio.h" // kernel printf implementation
#include "kernel_symbols.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

ISRHandler g_isr_handlers[IDT_SIZE];

static const char *const g_exceptions[] = {"Divide by zero error",
                                           "Debug",
                                           "Non-maskable Interrupt",
                                           "Breakpoint",
                                           "Overflow",
                                           "Bound Range Exceeded",
                                           "Invalid Opcode",
                                           "Device Not Available",
                                           "Double Fault",
                                           "Coprocessor Segment Overrun",
                                           "Invalid TSS",
                                           "Segment Not Present",
                                           "Stack-Segment Fault",
                                           "General Protection Fault",
                                           "Page Fault",
                                           "",
                                           "x87 Floating-Point Exception",
                                           "Alignment Check",
                                           "Machine Check",
                                           "SIMD Floating-Point Exception",
                                           "Virtualization Exception",
                                           "Control Protection Exception ",
                                           "",
                                           "",
                                           "",
                                           "",
                                           "",
                                           "",
                                           "Hypervisor Injection Exception",
                                           "VMM Communication Exception",
                                           "Security Exception",
                                           ""};

#include "arch/i686/gdt.h"

typedef void (*ty)(void);
#define DECLARE_ISR(n) extern void i686_isr##n(void);
#define ISR_LIST(_)                                                                                                                                                                                                                                                                                                                                                                                                \
  _(0)                                                                                                                                                                                                                                                                                                                                                                                                             \
  _(1)                                                                                                                                                                                                                                                                                                                                                                                                             \
  _(2)                                                                                                                                                                                                                                                                                                                                                                                                             \
  _(3) _(4) _(5) _(6) _(7) _(8) _(9) _(10) _(11) _(12) _(13) _(14) _(15) _(16)                                                                                                                                                                                                                                                                                                                                     \
      _(17) _(18) _(19) _(20) _(21) _(22) _(23) _(24) _(25) _(26) _(27) _(28)                                                                                                                                                                                                                                                                                                                                      \
          _(29) _(30) _(31) _(32) _(33) _(34) _(35) _(36) _(37) _(38) _(39)                                                                                                                                                                                                                                                                                                                                        \
              _(40) _(41) _(42) _(43) _(44) _(45) _(46) _(47) _(48) _(49)                                                                                                                                                                                                                                                                                                                                          \
                  _(50) _(51) _(52) _(53) _(54) _(55) _(56) _(57) _(58) _(59)                                                                                                                                                                                                                                                                                                                                      \
                      _(60) _(61) _(62) _(63) _(64) _(65) _(66) _(67) _(68)                                                                                                                                                                                                                                                                                                                                        \
                          _(69) _(70) _(71) _(72) _(73) _(74) _(75) _(                                                                                                                                                                                                                                                                                                                                             \
                              76) _(77) _(78) _(79) _(80) _(81) _(82) _(83) _(84) _(85) _(86) _(87) _(88) _(89) _(90) _(91) _(92) _(93) _(94) _(95) _(96) _(97) _(98) _(99) _(100) _(101) _(102) _(103) _(104) _(105) _(106) _(107) _(108) _(109) _(110) _(111) _(112) _(113) _(114) _(115) _(116) _(117) _(118) _(119) _(120) _(121) _(122) _(123) _(124) _(125) _(126) _(127) _(128) _(129) _(130) _(131) _(132) \
                              _(133) _(134) _(135) _(                                                                                                                                                                                                                                                                                                                                                              \
                                  136) _(137) _(138) _(139) _(140)                                                                                                                                                                                                                                                                                                                                                 \
                                  _(141) _(142) _(143) _(                                                                                                                                                                                                                                                                                                                                                          \
                                      144) _(145) _(146) _(147) _(148)                                                                                                                                                                                                                                                                                                                                             \
                                      _(149) _(150) _(151) _(                                                                                                                                                                                                                                                                                                                                                      \
                                          152) _(153) _(154) _(155) _(156)                                                                                                                                                                                                                                                                                                                                         \
                                          _(157) _(158) _(159) _(                                                                                                                                                                                                                                                                                                                                                  \
                                              160) _(161) _(162) _(163) _(164)                                                                                                                                                                                                                                                                                                                                     \
                                              _(165) _(166) _(167) _(                                                                                                                                                                                                                                                                                                                                              \
                                                  168) _(169) _(170) _(171) _(172)                                                                                                                                                                                                                                                                                                                                 \
                                                  _(173) _(174) _(175) _(                                                                                                                                                                                                                                                                                                                                          \
                                                      176) _(177) _(178) _(179) _(180)                                                                                                                                                                                                                                                                                                                             \
                                                      _(181) _(182) _(183) _(                                                                                                                                                                                                                                                                                                                                      \
                                                          184) _(185) _(186) _(187) _(188)                                                                                                                                                                                                                                                                                                                         \
                                                          _(189) _(190) _(191) _(                                                                                                                                                                                                                                                                                                                                  \
                                                              192) _(193) _(194) _(195) _(196)                                                                                                                                                                                                                                                                                                                     \
                                                              _(197) _(198) _(199) _(                                                                                                                                                                                                                                                                                                                              \
                                                                  200) _(201) _(202) _(203) _(204)                                                                                                                                                                                                                                                                                                                 \
                                                                  _(205) _(206) _(207) _(                                                                                                                                                                                                                                                                                                                          \
                                                                      208) _(209) _(210) _(211) _(212)                                                                                                                                                                                                                                                                                                             \
                                                                      _(213) _(214) _(215) _(                                                                                                                                                                                                                                                                                                                      \
                                                                          216) _(217) _(218) _(219) _(220)                                                                                                                                                                                                                                                                                                         \
                                                                          _(221) _(222) _(223) _(                                                                                                                                                                                                                                                                                                                  \
                                                                              224) _(225) _(226) _(227)                                                                                                                                                                                                                                                                                                            \
                                                                              _(228) _(229) _(230) _(                                                                                                                                                                                                                                                                                                              \
                                                                                  231) _(232) _(233) _(234)                                                                                                                                                                                                                                                                                                        \
                                                                                  _(235) _(236) _(237) _(                                                                                                                                                                                                                                                                                                          \
                                                                                      238) _(239) _(240)                                                                                                                                                                                                                                                                                                           \
                                                                                      _(241) _(242) _(                                                                                                                                                                                                                                                                                                             \
                                                                                          243) _(244) _(245)                                                                                                                                                                                                                                                                                                       \
                                                                                          _(246) _(247) _(                                                                                                                                                                                                                                                                                                         \
                                                                                              248) _(249)                                                                                                                                                                                                                                                                                                          \
                                                                                              _(250) _(                                                                                                                                                                                                                                                                                                            \
                                                                                                  251)                                                                                                                                                                                                                                                                                                             \
                                                                                                  _(252) _(                                                                                                                                                                                                                                                                                                        \
                                                                                                      253)                                                                                                                                                                                                                                                                                                         \
                                                                                                      _(254)

ISR_LIST(DECLARE_ISR)
#define ADD_TO_ARRAY(n) i686_isr##n,

void i686_isr_init(void) {
  // Build array of handlers
  ty isr_handlers[] = {ISR_LIST(ADD_TO_ARRAY)};

  int count = sizeof(isr_handlers) / sizeof(isr_handlers[0]);
  for (int i = 0; i < count; i++) {
    uint32_t flags = IDT_FLAGS_RING0 | IDT_FLAGS_GATE_TRAP_32b;
    if (i == 45)
      flags = IDT_FLAGS_RING3 | IDT_FLAGS_GATE_TRAP_32b;

    i686_idt_gate_set(i, isr_handlers[i], i686_GDT_KERNEL_CS_SEL, flags);
  }
}

static void panic_ebp_backtrace(uintptr_t *ebp) {
  debugf_and_printf("Backtrace (EBP chain):\n");

  int depth = 0;
  uintptr_t *cur_ebp = ebp; // keep a copy for tool output
  while (cur_ebp && depth < 20) {
    uintptr_t ret = cur_ebp[1];
    if (ret == 0)
      break;
    debugf_and_printf("  #%d: 0x%x <%s>\n", depth, (unsigned)ret,
                      lookup_symbol(ret));
    cur_ebp = (uintptr_t *)cur_ebp[0];
    depth++;
  }
}

// for than the run.py to use with addr2line tool
void debugf_stacktrace_line(uintptr_t *ebp) {
  debugf("STACK_OF_PANIC[123]:");
  int depth = 0;
  uintptr_t *cur_ebp = ebp; // keep a copy for tool output
  while (cur_ebp && depth < 20) {
    uintptr_t ret = cur_ebp[1];
    if (ret == 0)
      break;
    debugf(" 0x%x", (unsigned)ret);
    cur_ebp = (uintptr_t *)cur_ebp[0];
    depth++;
  }
  debugf("\n");
}

void i686_isr_handler(Registers *regs) {
  if (g_isr_handlers[regs->interrupt] != NULL)
    g_isr_handlers[regs->interrupt](regs);
  else if (regs->interrupt >= 32)
    debugf_and_printf("Unhandled interrupt %d!\n", regs->interrupt);
  else {
    debugf_and_printf("Unhandled exception %d %s\n", regs->interrupt,
                      g_exceptions[regs->interrupt]);
    debugf_and_printf("  eax=%x  ebx=%x  ecx=%x  edx=%x  esi=%x  edi=%x\n",
                      regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi,
                      regs->edi);

    debugf_and_printf(
        "  esp=%x  ebp=%x  eip=%x  eflags=%x  cs=%x  ds=%x  ss=%x\n",
        regs->esp_user, regs->ebp, regs->eip, regs->eflags, regs->cs, regs->ds,
        regs->ss_user);
    debugf_and_printf("  interrupt=%x  errorcode=%x\n", regs->interrupt,
                      regs->error);

    if ((regs->cs & 0x3) == 0) {
      debugf("FAULTING_INSTRUCTION_OF_PANIC[123]: 0x%x\n", (unsigned)regs->eip);

      debugf_stacktrace_line((uintptr_t *)regs->ebp);
      panic_ebp_backtrace((uintptr_t *)regs->ebp);

    } else {
      debugf("User space exception at eip=0x%x\n", regs->eip);
      debugf("Cannot trace user stack from kernel\n");
    }
    arch_panic_halt();
  }
}

void i686_isr_handler_register(int interrupt, ISRHandler handler) {
  g_isr_handlers[interrupt] = handler;
  i686_idt_gate_enable(interrupt);
}
