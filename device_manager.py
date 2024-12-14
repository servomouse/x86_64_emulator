import ctypes
import log_manager
import copy
import os
import glob
import zipfile
from datetime import datetime

import platform
os_type = platform.system()
if os_type == 'Windows':
    import pefile   # pip install pefile
elif os_type == 'Linux':
    from elftools.elf.elffile import ELFFile


def get_type(item):
    if item == "void":
        return None
    if item == "int":
        return ctypes.c_int
    if item == "uint8_t":
        return ctypes.c_uint8
    if item == "uint16_t":
        return ctypes.c_uint16
    if item == "uint32_t":
        return ctypes.c_uint32
    print(f"ERROR: Unknown type: {item}")
    os.exit(1)


def get_dll_function(dll_object, signature: str):
    ''' Signature looks like this: "void foo (int, uint8_t)"
        where "foo" is the actual name of the function '''
    remove_symbols = [',', '(', ')']
    for s in remove_symbols:
        signature = signature.replace(s, ' ')
    # signature = signature.replace('(', ' ')
    # signature = signature.replace(')', ' ')
    items = signature.split()
    res_type = get_type(items[0])
    params = []
    for i in items[2:]:
        params.append(get_type(i))
    foo = getattr(dll_object, items[1])
    if params[0] is None:
        foo.argtypes = None
    else:
        foo.argtypes = params
    foo.restype = res_type
    return foo


def get_functions_list(dll_filename):
    symbols = []
    if os_type == 'Windows':
        pe_obj = pefile.PE(dll_filename)
        for exp in pe_obj.DIRECTORY_ENTRY_EXPORT.symbols:
            symbols.append(exp.name)
            # print(hex(pe_obj.OPTIONAL_HEADER.ImageBase + exp.address), exp.name)
    
    elif os_type == 'Linux':
        with open(dll_filename, 'rb') as f:
            elffile = ELFFile(f)
            symtab = elffile.get_section_by_name('.symtab')
            if symtab:
                for symbol in symtab.iter_symbols():
                    symbols.append(symbol.name)
                    print(symbol.name, symbol.entry.st_info.type)
    return symbols


class CommonDevModule():
    def __init__(self, filename):
        self.id = 0
        self.filename = filename
        self.device = ctypes.CDLL(filename)
        # Log output function
        self.device.set_log_func.argtypes = [log_manager.print_callback_t]
        self.device.set_log_func.restype = None
        self.device.set_log_func(log_manager.print_callback)

        self.set_log_level = get_dll_function(self.device, "void set_log_level(uint8_t)")

        self.module_reset = get_dll_function(self.device, "void module_reset(void)")
        self.module_reset()

        self.module_save = get_dll_function(self.device, "void module_save(void)")
        self.module_restore = get_dll_function(self.device, "void module_restore(void)")
        self.module_tick = get_dll_function(self.device, "int module_tick(uint32_t)")
    

class ReadWriteModule:
    def set_read_write_functions(self):
        self.data_write = get_dll_function(self.device, "void data_write(uint32_t, uint16_t, uint8_t)")
        self.data_read = get_dll_function(self.device, "uint16_t data_read(uint32_t, uint8_t)")
        #Create functions pointers:
        self.data_write_p = ctypes.CFUNCTYPE(None, ctypes.c_uint32, ctypes.c_uint16, ctypes.c_uint8)(self.device.data_write)
        self.data_read_p = ctypes.CFUNCTYPE(ctypes.c_uint16, ctypes.c_uint32, ctypes.c_uint8)(self.device.data_read)
        try:
            # self.device.code_read.argtypes = [ctypes.c_uint32, ctypes.c_uint8]
            # self.device.code_read.restype = ctypes.c_uint16
            # self.code_read = self.device.code_read
            self.code_read = get_dll_function(self.device, "uint16_t code_read(uint32_t, uint8_t)")
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
        # self.device.cpu_get_ticks.argtypes = None
        # self.device.cpu_get_ticks.restype = ctypes.c_uint32
        # self.cpu_get_ticks = self.device.cpu_get_ticks
        self.cpu_get_ticks = get_dll_function(self.device, "uint32_t cpu_get_ticks(void)")


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
        files = glob.glob('./data/*.bin')
        now = datetime.now() # current date and time
        date_time = now.strftime("%m-%d-%Y_%H-%M-%S")

        with zipfile.ZipFile(f"data/state_{date_time}.zip", mode="w") as archive:
            for f in files:
                archive.write(f)
    
    def restore_devices(self):
        for _, dev in self.devices.items():
            dev.module_restore()
    
    def tick_devices(self):
        """ On fail saves devices and returns False """
        self._ticks += 1
        for dev_name, dev in self.devices.items():
            try:
                if 0 != dev.module_tick(self._ticks):
                    self.save_devices()
                    return False
            except Exception as e:
                print(f"Error ticking device {dev_name}!")
                print(e)
                return False
            # if dev_name == 'cpu':
            #     self._ticks = dev.cpu_get_ticks()
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