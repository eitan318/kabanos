import gdb
import os


class LoadUserSymbols(gdb.Command):
    """Read g_exec_table from kernel and add-symbol-file for each entry."""

    def __init__(self):
        super().__init__("load-user-syms", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        # Base path where your ELFs live on the host
        elf_host_base = (
            arg.strip() if arg.strip() else "/home/magshimim/repos/10001_myos/BOOT"
        )

        count = int(gdb.parse_and_eval("g_exec_table_count"))
        print(f"[load-user-syms] {count} entries in g_exec_table")

        for i in range(count):
            entry = gdb.parse_and_eval(f"g_exec_table[{i}]")
            loaded = int(entry["loaded"])
            if not loaded:
                continue

            path = entry["path"].string()
            load_base = int(entry["load_base"])

            # Map kernel-side path to host path
            # e.g. /boot/init.elf -> /path/to/BOOT/init.elf
            filename = os.path.basename(path)
            host_path = os.path.join(elf_host_base, filename)

            if not os.path.exists(host_path):
                print(f"[load-user-syms] WARNING: {host_path} not found, skipping")
                continue

            cmd = f"add-symbol-file {host_path} 0x{load_base:x}"
            print(f"[load-user-syms] {cmd}")
            gdb.execute(cmd)


LoadUserSymbols()
