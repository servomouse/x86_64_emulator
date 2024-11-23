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

common_flags = ["-lm", "-g", "-Wall", "-lws2_32", "-I.", "-I./devices", "-I./tests", "utils.c"]


def get_config(filename: str):
    with open(filename, 'r') as f:
        targets = toml.load(f)
    return targets


def build_target(target, config):
    if len(config[target]['module'][0]) > 0:
        print(f"Building {target} . . . ", end='', flush=True)
        fname = f"bin/{target}.dll"
        if os.path.isfile(fname):
            os.remove(fname)
        srcs = ' '.join(config[target]['module'])
        flags = ' '.join(common_flags)
        output = subprocess.getoutput(f"gcc -shared {flags} {srcs} -o {fname}")
        if len(output) == 0:
            print("Done")
        else:
            print(f"Failed\nERROR: {output}")
    if len(config[target]['tests'][0]) > 0:
        print(f"Building tests for {target} . . . ", end='', flush=True)
        fname = f"bin/test_{target}.dll"
        if os.path.isfile(fname):
            os.remove(fname)
        srcs = ' '.join(config[target]['module']) + ' ' + ' '.join(config[target]['tests'])
        flags = ' '.join(common_flags)
        output = subprocess.getoutput(f"gcc {flags} {srcs} -o {fname}")
        if len(output) == 0:
            print("Done")
        else:
            print(f"Failed\nERROR: {output}")


def build(target):
    config = get_config("config.toml")
    if target == "all":
        for target in config:
            build_target(target, config)
    else:
        if target in config:
            build_target(target, config)
        else:
            print(f"ERROR: Target {target} not found!")



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
            build("all")
        else:
            build(sys.argv[1])
    else:
        build("all")


if __name__ == "__main__":
    # print(get_config("targets.toml"))
    main()
