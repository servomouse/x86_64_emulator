import subprocess
import os
import sys
import glob
import toml     # pip install toml

# gcc -shared -o bin/8259a_int_controller.dll devices/8259a_interrupt_controller.c utils.c log_server_iface/logs_win.c -lws2_32 -I. -I./devices
# gcc -shared -o bin/8259a_int_controller.dll devices/8259a_interrupt_controller.c utils.c -I. -I./devices -Wall
# gcc -shared -o bin/8086_io.dll 8086_io.c utils.c -I. -I./devices -Wall
# gcc -shared -o bin/8086_mem.dll 8086_mem.c utils.c -I. -I./devices -Wall
# gcc -shared -o bin/8086_cpu.dll 8086.c utils.c -I. -I./devices -I./tests -Wall

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
flags = ["-lm", "-g", "-Wall", "-lws2_32", "-I.", "-I./devices", "-I./tests"]


def get_targets(filename: str):
    with open(filename, 'r') as f:
        targets = toml.load(f)
    return targets


def build(target):
    targets = get_targets("targets.toml")
    if target == "all":
        pass
    else:
        if target in targets and len(targets[target]['module'][0]) > 0:
            pass



def build_all():
    files = glob.glob('./bin/*')
    for f in files:
        os.remove(f)
    files = glob.glob('./devices/*.c')
    for f in files:
        fname = os.path.basename(f).replace(".c", "")
        print(f"Building {fname} . . . ", end='', flush=True)
        output = subprocess.getoutput(f"gcc -shared -o bin/{fname}.dll devices/{fname}.c utils.c -I. -I./devices -Wall")
        if len(output) == 0:
            print("Done")
        else:
            print(f"Failed\nERROR: {output}")
            sys.exit(-1)


def main():
    if len(sys.argv) > 1:
        if sys.argv[1] == "all":
            build_all()
    else:
        build_all()


if __name__ == "__main__":
    print(get_targets("targets.toml"))
    # main()
