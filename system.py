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
        "int_ctrlr": {"file": "bin/8259a_interrupt_controller.dll", "type": "device"},
        "dma_ctrlr": {"file": "bin/8237a-5_dma.dll", "type": "device"},
        "cga_ctrlr": {"file": "bin/8086_cga.dll", "type": "device"},
        "mda_ctrlr": {"file": "bin/8086_mda.dll", "type": "device"},
        "ppi_ctrlr": {"file": "bin/8255a-5_ppi.dll", "type": "device"},
        "timer": {"file": "bin/8253_timer.dll", "type": "device"},
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
        "nmi_wire": {"devices": ["cpu", "int_ctrlr"], "default_state": WireState.WIRE_LOW},
        "int_wire": {"devices": ["cpu", "int_ctrlr"], "default_state": WireState.WIRE_LOW},
        "int0_wire": {"devices": ["timer", "int_ctrlr"], "default_state": WireState.WIRE_LOW},
        "ch1_output_wire": {"devices": ["timer"], "default_state": WireState.WIRE_LOW}, # Dummy wire
        "ch2_output_wire": {"devices": ["timer"], "default_state": WireState.WIRE_LOW}, # Dummy wire
        "int1_wire": {"devices": ["ppi_ctrlr", "int_ctrlr"], "default_state": WireState.WIRE_LOW},
    }

    for wire_name, config in wires.items():
        temp_wire = Wire(wire_name)
        for dev in config["devices"]:
            temp_wire.connect_device(mb.devices[dev].device)
        # temp_wire.connect_device(mb.devices["int_ctrlr"].device)
        temp_wire.set_state(config["default_state"])

    int_ctrlr = mb.devices["int_ctrlr"]
    dma_ctrlr = mb.devices["dma_ctrlr"]
    cga_ctrlr = mb.devices["cga_ctrlr"]
    mda_ctrlr = mb.devices["mda_ctrlr"]
    ppi_ctrlr = mb.devices["ppi_ctrlr"]
    timer = mb.devices["timer"]
    io_ctrlr = mb.devices["io_ctrlr"]

    int_ctrlr.id0 = io_ctrlr.map_device(int_ctrlr.addr_start, int_ctrlr.addr_end, int_ctrlr.data_write_p, int_ctrlr.data_read_p)
    int_ctrlr.id1 = io_ctrlr.map_device(0x20, 0x21, int_ctrlr.data_write_p, int_ctrlr.data_read_p)
    dma_ctrlr.id0 = io_ctrlr.map_device(dma_ctrlr.addr_start, dma_ctrlr.addr_end, dma_ctrlr.data_write_p, dma_ctrlr.data_read_p)
    dma_ctrlr.id1 = io_ctrlr.map_device(0x00, 0x0F, dma_ctrlr.data_write_p, dma_ctrlr.data_read_p)
    cga_ctrlr.id = io_ctrlr.map_device(cga_ctrlr.addr_start, cga_ctrlr.addr_end, cga_ctrlr.data_write_p, cga_ctrlr.data_read_p)
    mda_ctrlr.id = io_ctrlr.map_device(mda_ctrlr.addr_start, mda_ctrlr.addr_end, mda_ctrlr.data_write_p, mda_ctrlr.data_read_p)
    ppi_ctrlr.id = io_ctrlr.map_device(ppi_ctrlr.addr_start, ppi_ctrlr.addr_end, ppi_ctrlr.data_write_p, ppi_ctrlr.data_read_p)
    timer.id = io_ctrlr.map_device(timer.addr_start, timer.addr_end, timer.data_write_p, timer.data_read_p)

    mb.devices["cpu"].connect_address_space(0, mb.devices["io_ctrlr"].data_write_p, mb.devices["io_ctrlr"].data_read_p)
    mb.devices["cpu"].connect_address_space(1, mb.devices["memory"].data_write_p, mb.devices["memory"].data_read_p)
    mb.devices["cpu"].set_code_read_func(mb.devices["memory"].code_read_p)


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
