import ctypes


CALLBACK = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_char_p)

def print_logs(filename, logstring):
    print(f"{filename} <= {logstring}")

callback = CALLBACK(print_logs)



class WireType(ctypes.c_int):
    WIRE_INPUT = 0
    WIRE_OUTPUT_PP = 1
    WIRE_OUTPUT_OC = 2


class WireState(ctypes.c_int):
    WIRE_LOW = 0
    WIRE_HIGH = 1


class Wire:
    def __init__(self, name):
        self.name = name
        self.state = WireState.WIRE_LOW
        self.connections = []
    
    def get_state(self):
        return self.state
    
    def set_state(self, new_state):
        if self.state == new_state:
            return
        print(f"{self.name}: setting new state to {new_state}")
        self.state = new_state
        for conn in self.connections:
            conn.wire_state_change_cb(new_state)
    
    def connect_device(self, device):
        @ctypes.CFUNCTYPE(WireState)
        def _wire_get_state():
            return self.state

        @ctypes.CFUNCTYPE(None, WireState)
        def _wire_set_state(new_state):
            self.set_state(new_state)

        class WireStruct(ctypes.Structure):
            _fields_ = [("wire_type", WireType),
                        ("wire_get_state", ctypes.CFUNCTYPE(WireState)),
                        ("wire_set_state", ctypes.CFUNCTYPE(None, WireState)),
                        ("wire_state_change_cb", ctypes.CFUNCTYPE(None, WireState))]

        wire_struct = WireStruct.in_dll(device, self.name)
        wire_struct.wire_get_state = _wire_get_state
        wire_struct.wire_set_state = _wire_set_state
        self.connections.append(wire_struct)


class CommonDevModule():
    def __init__(self, filename):
        self.id = 0
        self.filename = filename
        self.device = ctypes.CDLL(filename)
        # Log function
        self.device.set_log_func.argtypes = [CALLBACK]
        self.device.set_log_func.restype = None
        self.device.set_log_func(callback)

        self.device.module_reset.argtypes = None
        self.device.module_reset.restype = None
        self.module_reset = self.device.module_reset
        self.module_reset()

        self.device.module_save.argtypes = None
        self.device.module_save.restype = None
        self.module_save = self.device.module_save

        self.device.module_restore.argtypes = None
        self.device.module_restore.restype = None
        self.module_restore = self.device.module_restore

        self.device.module_tick.argtypes = None
        self.device.module_tick.restype = ctypes.c_int
        self.module_tick = self.device.module_tick
    

class ReadWriteModule:
    def set_read_write_functions(self):
        self.device.data_write.argtypes = [ctypes.c_uint32, ctypes.c_uint16, ctypes.c_uint8]
        self.device.data_write.restype = None
        self.data_write = self.device.data_write

        self.device.data_read.argtypes = [ctypes.c_uint32, ctypes.c_uint8]
        self.device.data_read.restype = ctypes.c_uint16
        self.data_read = self.device.data_read
        #Create functions pointers:
        self.data_write_p = ctypes.CFUNCTYPE(None, ctypes.c_uint32, ctypes.c_uint16, ctypes.c_uint8)(self.device.data_write)
        self.data_read_p = ctypes.CFUNCTYPE(ctypes.c_uint16, ctypes.c_uint32, ctypes.c_uint8)(self.device.data_read)


class DevModule(CommonDevModule, ReadWriteModule):
    def __init__(self, filename):
        super().__init__(filename)
        self.device.module_get_address_range.argtypes = None
        self.device.module_get_address_range.restype = ctypes.POINTER(ctypes.c_uint32)
        range_ptr = self.device.module_get_address_range()
        print(f"Device address range: {hex(range_ptr[0])} - {hex(range_ptr[1])}")
        self.addr_start = range_ptr[0]
        self.addr_end = range_ptr[1]
        self.set_read_write_functions()


class AddressSpace(CommonDevModule, ReadWriteModule):
    def __init__(self, filename):
        super().__init__(filename)
        write_func_t = ctypes.CFUNCTYPE(None, ctypes.c_uint32, ctypes.c_uint16, ctypes.c_uint8)
        read_func_t = ctypes.CFUNCTYPE(ctypes.c_uint16, ctypes.c_uint32, ctypes.c_uint8)
        self.device.map_device.argtypes = [ctypes.c_uint32, ctypes.c_uint32, write_func_t, read_func_t]
        self.device.map_device.restype = ctypes.c_uint32
        self.map_device = self.device.map_device
        self.set_read_write_functions()


class Processor(CommonDevModule):
    def __init__(self, filename):
        super().__init__(filename)
        write_func_t = ctypes.CFUNCTYPE(None, ctypes.c_uint32, ctypes.c_uint16, ctypes.c_uint8)
        read_func_t = ctypes.CFUNCTYPE(ctypes.c_uint16, ctypes.c_uint32, ctypes.c_uint8)
        self.device.connect_address_space.argtypes = [ctypes.c_uint8, write_func_t, read_func_t]
        self.device.connect_address_space.restype = None
        self.connect_address_space = self.device.connect_address_space


class DevManager():
    def __init__(self):
        self.devices = {}
    
    def add_device(self, device, dev_name):
        self.devices[dev_name] = device
    
    def reset_devices(self):
        for _, dev in self.devices.items():
            dev.module_reset()
    
    def save_devices(self):
        for _, dev in self.devices.items():
            dev.module_save()
    
    def restore_devices(self):
        for _, dev in self.devices.items():
            dev.module_restore()


# Load the shared libraries:
# Linux:
# libfile1 = ctypes.CDLL('./libfile1.so')  # Use 'file1.dll' on Windows
# libfile2 = ctypes.CDLL('./libfile2.so')  # Use 'file2.dll' on Windows
# Windows:
mb = DevManager()
mb.add_device(DevModule("bin/8259a_int_controller.dll"), "int_ctrlr")
mb.add_device(AddressSpace("bin/8086_io.dll"), "io_ctrlr")
mb.add_device(AddressSpace("bin/8086_mem.dll"), "memory")
mb.add_device(Processor("bin/8086_cpu.dll"), "cpu")

nmi_wire = Wire("nmi_wire")
nmi_wire.connect_device(mb.devices["cpu"].device)
nmi_wire.connect_device(mb.devices["int_ctrlr"].device)

int_wire = Wire("int_wire")
int_wire.connect_device(mb.devices["cpu"].device)
int_wire.connect_device(mb.devices["int_ctrlr"].device)

nmi_wire.set_state(WireState.WIRE_HIGH)
int_wire.set_state(WireState.WIRE_HIGH)
# int_controller = ctypes.CDLL("bin/8259a_int_controller.dll")  # Use 'libfile1.so' on Linux
# io_controller = ctypes.CDLL("bin/8086_io.dll")  # Use 'libfile1.so' on Linux
# libfile2 = ctypes.CDLL('./libfile2.so')  # Use 'libfile2.so' on Linux


# Define the structure as per the C definition
# class MyStruct(ctypes.Structure):
#     _fields_ = [("a", ctypes.c_int), ("b", ctypes.c_double)]

# Access the global my_struct from libfile1
# my_struct = MyStruct.in_dll(libfile1, "my_struct")

# Define the function prototypes
# int_controller.set_log_func.argtypes = [CALLBACK]
# int_controller.set_log_func.restype = None

# int_controller.io_write.argtypes = [ctypes.c_uint32, ctypes.c_uint16, ctypes.c_uint8]
# int_controller.io_write.restype = None

# int_controller.io_read.argtypes = [ctypes.c_uint32, ctypes.c_uint8]
# int_controller.io_read.restype = ctypes.c_uint16

# int_controller.module_reset.argtypes = None
# int_controller.module_reset.restype = None

# int_controller.module_save.argtypes = None
# int_controller.module_save.restype = None

# int_controller.module_restore.argtypes = None
# int_controller.module_restore.restype = None
# int_ctrlr = DevModule("bin/8259a_int_controller.dll")
# io_ctrlr = AddressSpace("bin/8086_io.dll")

# libfile2.func.argtypes = [ctypes.CFUNCTYPE(None, ctypes.c_int), ctypes.CFUNCTYPE(ctypes.c_int), ctypes.POINTER(MyStruct)]
# libfile2.func.restype = ctypes.c_int

# Create function pointers
# foo_func = ctypes.CFUNCTYPE(None, ctypes.c_int)(libfile1.foo)
# bar_func = ctypes.CFUNCTYPE(ctypes.c_int)(libfile1.bar)

# Call the function from the second shared object
# result = libfile2.func(foo_func, bar_func, ctypes.byref(my_struct))
# print("Result:", result)

# int_controller.set_log_func(callback)
# int_controller.io_write(0xA0, 19, 1)
# result = int_controller.io_read(0xA0, 1)
# print(result)
# int_controller.module_save()
# int_controller.module_reset()
# result = int_controller.io_read(0xA0, 1)
# print("After reset: ", result)
# int_controller.module_restore()
# result = int_controller.io_read(0xA0, 1)
# print("After restore: ", result)

int_ctrlr = mb.devices["int_ctrlr"]
io_ctrlr = mb.devices["io_ctrlr"]

int_ctrlr.id = io_ctrlr.map_device(int_ctrlr.addr_start, int_ctrlr.addr_end, int_ctrlr.data_write_p, int_ctrlr.data_read_p)

mb.devices["cpu"].connect_address_space(0, mb.devices["io_ctrlr"].data_write_p, mb.devices["io_ctrlr"].data_read_p)
mb.devices["cpu"].connect_address_space(1, mb.devices["memory"].data_write_p, mb.devices["memory"].data_read_p)

io_ctrlr.data_write(0xA0, 19, 1)
result = io_ctrlr.data_read(0xA0, 1)
print(result)
# io_ctrlr.module_save()
mb.save_devices()
mb.reset_devices()
result = io_ctrlr.data_read(0xA0, 1)
print("After reset: ", result)
mb.restore_devices()
result = io_ctrlr.data_read(0xA0, 1)
print("After restore: ", result)