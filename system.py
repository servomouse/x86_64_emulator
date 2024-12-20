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


def beep_wire_cb(new_state):
    if new_state == 1:
        log_manager.send_data_to_console({"controls": {"beep-led": "on"}})
    else:
        log_manager.send_data_to_console({"controls": {"beep-led": "off"}})


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
        "nmi_wire": {"devices": {"cpu": "nmi_pin", "intc": "nmi_pin"}, "default_state": 0, "state_change_callback": None},
        "int_wire": {"devices": {"cpu": "int_pin", "intc": "int_pin"}, "default_state": 0, "state_change_callback": None},
        "int0_wire": {"devices": {"timer": "ch0_output_pin", "intc": "int0_pin"}, "default_state": 0, "state_change_callback": None},
        "ch1_output_wire": {"devices": {"timer": "ch1_output_pin"}, "default_state": 0, "state_change_callback": None},
        "ch2_output_wire": {"devices": {"timer": "ch2_output_pin"}, "default_state": 0, "state_change_callback": None},
        "int1_wire": {"devices": {"ppi": "int1_pin", "intc": "int1_pin"}, "default_state": 0, "state_change_callback": None},
        "beep_wire": {"devices": {"ppi": "beep_pin"}, "default_state": 0, "state_change_callback": beep_wire_cb},
        "int6_wire": {"devices": {"fdc": "int6_pin", "intc": "int6_pin"}, "default_state": 0, "state_change_callback": None},
    }

    for wire_name, config in wires_map.items():
        temp_wire = Wire(wire_name, config["state_change_callback"])
        for dev, pin_name in config["devices"].items():
            temp_wire.connect_device(mb.devices[dev].device, pin_name, dev)
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
        
        mb.save_state_at(22_580_000)    # 20749786, 21423128
        # mb.set_log_level_at(['timer', 10, 0])
        mb.set_log_level_at(['cpu', 22_580_000, 0])
        mb.set_log_level_at(['fdc', 22_580_000, 0])
        # mb.set_log_level_at(['intc', 10, 0])
        mb.set_log_level_at(['ioc', 22_580_000, 0])
        mb.set_log_level_at(['serial_port', 22_580_000, 0])
        mb.set_log_level_at(['printer', 22_580_000, 0])
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
