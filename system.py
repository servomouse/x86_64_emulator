import log_manager
import os
import sys
import time

from wires import (WireType, Wire)
from device_manager import (DevModule, AddressSpace, Processor, DevManager)
from build import get_config


stop_main_thread = False
main_thread = None
mb = None
wires = []


def system_init():
    global mb
    config = get_config("config.toml")

    # Instantiate devices
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
    
    # Map devices
    ioc = mb.devices["ioc"]
    for dev_name, dev_config in config.items():
        if dev_config['type'] == 'device':
            dev = mb.devices[dev_name]
            for addr_range in dev_config["address_ranges"]:
                print(f"Mapping device {dev_name} at address range: {hex(addr_range[0])} - {hex(addr_range[1])}")
                ioc.map_device(addr_range[0], addr_range[1], dev.data_write_p, dev.data_read_p)

    wires_map = {
        "nmi_wire": {"devices": ["cpu", "intc"], "default_state": 0},
        "int_wire": {"devices": ["cpu", "intc"], "default_state": 0},
        "int0_wire": {"devices": ["timer", "intc"], "default_state": 0},
        "ch1_output_wire": {"devices": ["timer"], "default_state": 0},
        "ch2_output_wire": {"devices": ["timer"], "default_state": 0},
        "int1_wire": {"devices": ["ppi", "intc"], "default_state": 0},
        "int6_wire": {"devices": ["fdc", "intc"], "default_state": 0},
    }

    for wire_name, config in wires_map.items():
        temp_wire = Wire(wire_name)
        for dev in config["devices"]:
            temp_wire.connect_device(mb.devices[dev].device)
        temp_wire.set_state(config["default_state"])
        wires.append(temp_wire)

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
        
        mb.save_state_at(20_700_000)    # 20749786, 21423128
        mb.set_log_level_at(['cpu', 20_700_000, 0])
        mb.set_log_level_at(['fdc', 20_000_000, 0])
        mb.set_log_level_at(['intc', 10, 0])
        # mb.set_log_level_at(['memory', 10, 0])
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
