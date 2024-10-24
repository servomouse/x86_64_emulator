# import ctypes
import log_manager
import signal
import os
import time
from wires import (WireType, WireState, Wire)
from device_manager import (DevModule, AddressSpace, Processor, DevManager)
import threading


stop_main_thread = False
main_thread = None
mb = None


def system_init():
    global mb
    devices = {
        "int_ctrlr": {"file": "bin/8259a_int_controller.dll", "type": "device"},
        "io_ctrlr": {"file": "bin/8086_io.dll", "type": "address_space"},
        "memory": {"file": "bin/8086_mem.dll", "type": "address_space"},
        "cpu": {"file": "bin/8086_cpu.dll", "type": "processor"},
    }
    for dev_name, config in devices.items():
        cls = None
        if config["type"] == "device":
            mb.add_device(DevModule(config["file"]), dev_name)
        elif config["type"] == "address_space":
            mb.add_device(AddressSpace(config["file"]), dev_name)
        elif config["type"] == "processor":
            mb.add_device(Processor(config["file"]), dev_name)
        else:
            print(f"Unknown device type: {config['type']}")
            os._exit(1)

    wires = {
        "nmi_wire": {"devices": ["cpu", "int_ctrlr"], "default_state": WireState.WIRE_HIGH},
        "int_wire": {"devices": ["cpu", "int_ctrlr"], "default_state": WireState.WIRE_HIGH},
    }

    for wire_name, config in wires.items():
        temp_wire = Wire(wire_name)
        for dev in config["devices"]:
            temp_wire.connect_device(mb.devices[dev].device)
        temp_wire.connect_device(mb.devices["int_ctrlr"].device)
        temp_wire.set_state(config["default_state"])

    int_ctrlr = mb.devices["int_ctrlr"]
    io_ctrlr = mb.devices["io_ctrlr"]

    int_ctrlr.id = io_ctrlr.map_device(int_ctrlr.addr_start, int_ctrlr.addr_end, int_ctrlr.data_write_p, int_ctrlr.data_read_p)

    mb.devices["cpu"].connect_address_space(0, mb.devices["io_ctrlr"].data_write_p, mb.devices["io_ctrlr"].data_read_p)
    mb.devices["cpu"].connect_address_space(1, mb.devices["memory"].data_write_p, mb.devices["memory"].data_read_p)


def test_system():
    global mb
    io_ctrlr = mb.devices["io_ctrlr"]
    io_ctrlr.data_write(0xA0, 19, 1)
    result = io_ctrlr.data_read(0xA0, 1)
    print(result)
    # io_ctrlr.module_save()
    # mb.devices["cpu"].module_tick()
    mb.save_devices()
    mb.reset_devices()
    result = io_ctrlr.data_read(0xA0, 1)
    print("After reset: ", result)
    mb.restore_devices()
    result = io_ctrlr.data_read(0xA0, 1)
    print("After restore: ", result)


def run_system(stop_cond):
    global mb
    while mb.tick_devices():
        time.sleep(0.1)
        if stop_cond():
            return
    exit_program()


def main_thread_init():
    global main_thread, stop_main_thread
    main_thread = threading.Thread(target=run_system, args=(lambda: stop_main_thread,))
    main_thread.start()


def main_thread_exit():
    global stop_main_thread, main_thread
    stop_main_thread = True
    try:
        main_thread.join()
    except RuntimeError:
        print("Main thread has been completed. ", end='')


def exit_program():
    global stop_main_thread, main_thread, mb
    print("Exit main thread . . . ", end='')
    main_thread_exit()
    print("Done")
    print("Saving devices . . . ", end='')
    mb.save_devices()
    print("Done")
    print("Exit print thread . . . ", end='')
    log_manager.log_manager_exit()
    print("Done")
    os._exit(0)


def main():
    global mb
    log_manager.log_manager_init()
    mb = DevManager()
    system_init()

    def signal_handler(sig, frame):
        print("Ctrl-C received, exiting")
        exit_program()

    signal.signal(signal.SIGINT, signal_handler)
    main_thread_init()
    try:
        while not stop_main_thread:
            time.sleep(1)
    except KeyboardInterrupt:
        exit_program()
    time.sleep(1)


if __name__ == "__main__":
    main()
