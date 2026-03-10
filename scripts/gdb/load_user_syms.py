import gdb
import os


class LoadUserSymbols(gdb.Command):
    """Read g_exec_table from kernel and add-symbol-file for each entry."""

    def __init__(self):
        super().__init__("load-user-syms", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        # Base path where your ELFs live on the host
        elf_host_base = arg.strip() if arg.strip() else "build/sysroot/bin/"

        # Use parse_and_eval to get the count
        try:
            count = int(gdb.parse_and_eval("g_exec_table_count"))
        except gdb.error:
            print("[load-user-syms] Error: g_exec_table_count not found.")
            return

        print(f"[load-user-syms] {count} entries in g_exec_table")

        for i in range(count):
            entry = gdb.parse_and_eval(f"g_exec_table[{i}]")

            # 1. Check loaded flag safely
            load_base = int(entry["load_base"])
            if load_base == 0:
                continue

            # 2. Safely extract the path string
            try:
                # Get the raw bytes and decode, ignoring errors to prevent the crash
                path = entry["path"].string()
                if not path:
                    continue
            except Exception as e:
                continue

            filename = os.path.basename(path)
            host_path = os.path.join(elf_host_base, filename)

            if not os.path.exists(host_path):
                print(f"[load-user-syms] Skipping: {host_path} not found")
                continue

            # 3. Use -s .text to be explicit for GDB

            # 3. Load symbols.
            # Note: We use -readnow to ensure GDB pulls them in immediately
            cmd = f"add-symbol-file {host_path} 0x{load_base:x}"
            print(f"[load-user-syms] {cmd}")
            # 'from_tty=False' prevents GDB from asking "add symbol file? (y or n)"
            gdb.execute(cmd, from_tty=False)


LoadUserSymbols()
