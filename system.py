import log_manager
import os
import sys
import time
import toml

from wires import (WireType, WireState, Wire)
from device_manager import (DevModule, AddressSpace, Processor, DevManager)
from build import get_config


stop_main_thread = False
main_thread = None
mb = None


def system_init():
    global mb
    config = get_config("config.toml")

    for dev_name, dev_config in config.items():
        so_name = f"bin/{dev_name}.dll"
        if dev_config["type"] == "device":
            mb.add_device(DevModule(so_name), dev_name)
        elif dev_config["type"] == "address_space":
            mb.add_device(AddressSpace(so_name), dev_name)
        elif dev_config["type"] == "processor":
            mb.add_device(Processor(so_name), dev_name)
        else:
            print(f"Unknown device type: {dev_config['type']}")
            os._exit(1)

    wires = {
        "nmi_wire": {"devices": ["cpu", "intc"], "default_state": WireState.WIRE_LOW},
        "int_wire": {"devices": ["cpu", "intc"], "default_state": WireState.WIRE_LOW},
        "int0_wire": {"devices": ["timer", "intc"], "default_state": WireState.WIRE_LOW},
        "ch1_output_wire": {"devices": ["timer"], "default_state": WireState.WIRE_LOW}, # Dummy wire
        "ch2_output_wire": {"devices": ["timer"], "default_state": WireState.WIRE_LOW}, # Dummy wire
        "int1_wire": {"devices": ["ppi", "intc"], "default_state": WireState.WIRE_LOW},
    }

    for wire_name, config in wires.items():
        temp_wire = Wire(wire_name)
        for dev in config["devices"]:
            temp_wire.connect_device(mb.devices[dev].device)
        temp_wire.set_state(config["default_state"])

    intc = mb.devices["intc"]
    dma = mb.devices["dma"]
    cga = mb.devices["cga"]
    mda = mb.devices["mda"]
    ppi = mb.devices["ppi"]
    timer = mb.devices["timer"]
    ioc = mb.devices["ioc"]
    io_exp_box = mb.devices["io_exp_box"]

    intc.id0 = ioc.map_device(intc.addr_start, intc.addr_end, intc.data_write_p, intc.data_read_p)
    intc.id1 = ioc.map_device(0x20, 0x21, intc.data_write_p, intc.data_read_p)
    io_exp_box.id0 = ioc.map_device(io_exp_box.addr_start, io_exp_box.addr_end, io_exp_box.data_write_p, io_exp_box.data_read_p)
    dma.id0 = ioc.map_device(dma.addr_start, dma.addr_end, dma.data_write_p, dma.data_read_p)
    dma.id1 = ioc.map_device(0x00, 0x0F, dma.data_write_p, dma.data_read_p)
    cga.id = ioc.map_device(cga.addr_start, cga.addr_end, cga.data_write_p, cga.data_read_p)
    mda.id = ioc.map_device(mda.addr_start, mda.addr_end, mda.data_write_p, mda.data_read_p)
    ppi.id = ioc.map_device(ppi.addr_start, ppi.addr_end, ppi.data_write_p, ppi.data_read_p)
    timer.id = ioc.map_device(timer.addr_start, timer.addr_end, timer.data_write_p, timer.data_read_p)

    mb.devices["cpu"].connect_address_space(0, mb.devices["ioc"].data_write_p, mb.devices["ioc"].data_read_p)
    mb.devices["cpu"].connect_address_space(1, mb.devices["memory"].data_write_p, mb.devices["memory"].data_read_p)
    mb.devices["cpu"].set_code_read_func(mb.devices["memory"].code_read_p)


def test_system():
    global mb
    ioc = mb.devices["ioc"]
    ioc.data_write(0xA0, 19, 1)
    result = ioc.data_read(0xA0, 1)
    print(result)
    # ioc.module_save()
    # mb.devices["cpu"].module_tick()
    mb.save_devices()
    mb.reset_devices()
    result = ioc.data_read(0xA0, 1)
    print("After reset: ", result)
    mb.restore_devices()
    result = ioc.data_read(0xA0, 1)
    print("After restore: ", result)


def exit_program():
    global stop_main_thread, main_thread, mb
    print("Saving devices . . . ", end='')
    mb.save_devices()
    print("Done")
    print("Exit print thread . . . ", end='')
    log_manager.log_manager_exit()
    stop_main_thread = True
    print("Done")
    # os._exit(0)


def main():
    global mb
    try:
        log_manager.log_manager_init()
        mb = DevManager()
        system_init()
        if len(sys.argv) > 1 and sys.argv[1] == "--continue":
            print("Restoring devices")
            mb.restore_devices()
    except Exception as e:
        print(e)
        exit_program()

    try:
        while mb.tick_devices():
            # time.sleep(0.1)
            if stop_main_thread:
                break
            if log_manager.stop_thread:
                break
    except KeyboardInterrupt:
        print("Ctrl-C received, exit")
    exit_program()


if __name__ == "__main__":
    main()
