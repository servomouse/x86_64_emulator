import subprocess
import os
import sys


files_to_build = ["main.c",
                  "8086.c",
                  "8086_io.c",
                  "8086_mem.c",
                  "8086_mda.c",
                  "8086_ppi.c",
                  "8253_timer.c",
                  "devices/8259a_interrupt_controller.c",
                  "utils.c",
                  "log_server_iface/logs_win.c"]
output_file_name = "main"
flags = ["-lm", "-g", "-Wall", "-lws2_32", "-I.", "-I./devices"]


def main():
    os.remove(output_file_name) if os.path.exists(output_file_name) else None
    os.remove(f"{output_file_name}.exe") if os.path.exists(f"{output_file_name}.exe") else None
    print(subprocess.getoutput(f"gcc {' '.join(files_to_build)} {' '.join(flags)} -o {output_file_name}"))


if __name__ == "__main__":
    main()