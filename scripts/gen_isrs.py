import sys


def main() -> None:
    # Check arguments
    # if len(sys.argv) < 3:
    #     print("Usage: python gen_isr.py <asm_file> <c_file>")
    #     return
    #
    asm_file_name = "src/kernel/arch/i686/isr/gen_isrs.inc"  # sys.argv[1]
    c_handler_decs_file_name = (
        "src/kernel/arch/i686/isr/gen_isr_handler_declerations.inc"  # sys.argv[2]
    )
    c_gate_sets_file_name = (
        "src/kernel/arch/i686/isr/gen_isr_gates_sets.inc"  # sys.argv[3]
    )

    isrs_with_err_code = [8, 10, 11, 12, 13, 14, 17, 21, 29, 30]
    msg_autogen = "!! this file is auto generated !!\n"

    # Write to the C files
    with open(c_handler_decs_file_name, "w") as c_handler_decs_file:
        c_handler_decs_file.write(f"//{msg_autogen}")
        c_handler_decs_file.write("\n// C ISR handler declarations\n")
        for i in range(256):
            c_handler_decs_file.write(f"void __attribute((cdecl)) i686_isr{i}();\n")

    with open(c_gate_sets_file_name, "w") as c_gate_sets_file:
        c_gate_sets_file.write(f"//{msg_autogen}")
        c_gate_sets_file.write("\n// C ISR gates sets\n")
        for i in range(256):
            c_gate_sets_file.write(
                f"i686_idt_gate_set({i}, i686_isr{i}, i686_GDT_CODE_SEGMENT, IDT_FLAGS_RING0 | IDT_FLAGS_GATE_TRAP_32b);\n"
            )

    # Write to the ASM file
    with open(asm_file_name, "w") as asm_file:
        asm_file.write(f";{msg_autogen}")
        asm_file.write("\n; Assembly ISR stubs\n")
        for i in range(256):
            if i in isrs_with_err_code:
                asm_file.write(f"ISR_ERRORCODE {i}\n")
            else:
                asm_file.write(f"ISR_NOERRORCODE {i}\n")


if __name__ == "__main__":
    main()
