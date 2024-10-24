import log_manager
import os
import sys
import time
from wires import (WireType, WireState, Wire)
from device_manager import (DevModule, AddressSpace, Processor, DevManager)


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


def exit_program():
    global stop_main_thread, main_thread, mb
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
    if len(sys.argv) > 1 and sys.argv[1] == "--continue":
        print("Restoring devices")
        mb.restore_devices()

    try:
        while mb.tick_devices():
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("Ctrl-C received, exit")
    exit_program()


if __name__ == "__main__":
    main()
