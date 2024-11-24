import ctypes
import log_manager
import copy


class CommonDevModule():
    def __init__(self, filename):
        self.id = 0
        self.filename = filename
        self.device = ctypes.CDLL(filename)
        # Log output function
        self.device.set_log_func.argtypes = [log_manager.print_callback_t]
        self.device.set_log_func.restype = None
        self.device.set_log_func(log_manager.print_callback)
        # Log-level function
        self.device.set_log_level.argtypes = [ctypes.c_uint8]
        self.device.set_log_level.restype = None
        self.set_log_level = self.device.set_log_level

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
        try:
            self.device.code_read.argtypes = [ctypes.c_uint32, ctypes.c_uint8]
            self.device.code_read.restype = ctypes.c_uint16
            self.code_read = self.device.code_read
            self.code_read_p = ctypes.CFUNCTYPE(ctypes.c_uint16, ctypes.c_uint32, ctypes.c_uint8)(self.device.code_read)
        except:
            self.code_read_p = None


class DevModule(CommonDevModule, ReadWriteModule):
    def __init__(self, filename):
        super().__init__(filename)
        # self.device.module_get_address_range.argtypes = None
        # self.device.module_get_address_range.restype = ctypes.POINTER(ctypes.c_uint32)
        # range_ptr = self.device.module_get_address_range()
        # print(f"Device address range: {hex(range_ptr[0])} - {hex(range_ptr[1])}")
        # self.addr_start = range_ptr[0]
        # self.addr_end = range_ptr[1]
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
        self.device.set_code_read_func.argtypes = [read_func_t]
        self.connect_address_space = self.device.connect_address_space
        self.set_code_read_func = self.device.set_code_read_func
        # Get tick counter function
        self.device.cpu_get_ticks.argtypes = None
        self.device.cpu_get_ticks.restype = ctypes.c_uint32
        self.cpu_get_ticks = self.device.cpu_get_ticks


class DevManager():
    def __init__(self):
        self.devices = {}
        self._save_state_at = 0
        self._set_log_level_at = []  # [device_name, ticks, new_log_level]
        self._ticks = 0
    
    def save_state_at(self, ticks):
        self._save_state_at = ticks
    
    def set_log_level_at(self, ticks):
        ''' ticks: [device_name, ticks, new_log_level] '''
        self._set_log_level_at.append(ticks)
    
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
    
    def tick_devices(self):
        """ On fail saves devices and returns False """
        for dev_name, dev in self.devices.items():
            try:
                if 0 != dev.module_tick():
                    self.save_devices()
                    return False
            except Exception as e:
                print(f"Error ticking device {dev_name}!")
                print(e)
                return False
            if dev_name == 'cpu':
                self._ticks = dev.cpu_get_ticks()
        if self._save_state_at > 0 and self._ticks == self._save_state_at:
            self.save_devices()
            print(f"Target ticks {self._save_state_at} reached, devices state saved!")
        if len(self._set_log_level_at) > 0:
            temp_arr = []
            need_update = False
            for i in self._set_log_level_at:
                if i[1] == self._ticks:
                    print(f"Setting log_level for {i[0]} to {i[2]}")
                    self.devices[i[0]].set_log_level(i[2])
                    need_update = True
                else:
                    temp_arr.append(i)
            if need_update:
                self._set_log_level_at = copy.deepcopy(temp_arr)

        return True