import gdb
import os


class LoadUserSymbols(gdb.Command):
    """Read g_exec_table from kernel and add-symbol-file for each entry."""

    def __init__(self):
        super().__init__("load-user-syms", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        # Base path where your ELFs live on the host
        elf_host_base = arg.strip() if arg.strip() else "build/sysroot/bin/"

        count = int(gdb.parse_and_eval("g_exec_table_count"))
        print(f"[load-user-syms] {count} entries in g_exec_table")

        for i in range(count):
            entry = gdb.parse_and_eval(f"g_exec_table[{i}]")

            # 1. Check loaded flag safely
            if int(entry["loaded"]) == 0:
                continue

            # 2. Safely extract the path string
            try:
                # Get the raw bytes and decode, ignoring errors to prevent the crash
                path_ptr = entry["path"]
                path = path_ptr.string(errors="replace")
            except Exception as e:
                print(f"[load-user-syms] Error reading path at index {i}: {e}")
                continue

            load_base = int(entry["load_base"])
            filename = os.path.basename(path)
            host_path = os.path.join(elf_host_base, filename)

            if not os.path.exists(host_path):
                print(f"[load-user-syms] Skipping: {host_path} not found")
                continue

            # 3. Use -s .text to be explicit for GDB
            cmd = f"add-symbol-file {host_path} -s .text 0x{load_base:x}"
            print(f"[load-user-syms] {cmd}")
            gdb.execute(cmd)


LoadUserSymbols()
