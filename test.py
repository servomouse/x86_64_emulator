import ctypes

# Load the shared libraries:
# Linux:
# libfile1 = ctypes.CDLL('./libfile1.so')  # Use 'file1.dll' on Windows
# libfile2 = ctypes.CDLL('./libfile2.so')  # Use 'file2.dll' on Windows
# Windows:
int_controller = ctypes.CDLL("bin/8259a_int_controller.dll")  # Use 'libfile1.so' on Linux
# libfile2 = ctypes.CDLL('./libfile2.so')  # Use 'libfile2.so' on Linux

CALLBACK = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_char_p)

def print_logs(filename, logstring):
    print(f"{filename} <= {logstring}")

callback = CALLBACK(print_logs)

# Define the structure as per the C definition
# class MyStruct(ctypes.Structure):
#     _fields_ = [("a", ctypes.c_int), ("b", ctypes.c_double)]

# Access the global my_struct from libfile1
# my_struct = MyStruct.in_dll(libfile1, "my_struct")

# Define the function prototypes
int_controller.set_log_func.argtypes = [CALLBACK]
int_controller.set_log_func.restype = None

int_controller.io_write.argtypes = [ctypes.c_uint32, ctypes.c_uint16, ctypes.c_uint8]
int_controller.io_write.restype = None

int_controller.io_read.argtypes = [ctypes.c_uint32, ctypes.c_uint8]
int_controller.io_read.restype = ctypes.c_uint16

int_controller.module_reset.argtypes = None
int_controller.module_reset.restype = None

int_controller.module_save.argtypes = None
int_controller.module_save.restype = None

int_controller.module_restore.argtypes = None
int_controller.module_restore.restype = None

# libfile2.func.argtypes = [ctypes.CFUNCTYPE(None, ctypes.c_int), ctypes.CFUNCTYPE(ctypes.c_int), ctypes.POINTER(MyStruct)]
# libfile2.func.restype = ctypes.c_int

# Create function pointers
# foo_func = ctypes.CFUNCTYPE(None, ctypes.c_int)(libfile1.foo)
# bar_func = ctypes.CFUNCTYPE(ctypes.c_int)(libfile1.bar)

# Call the function from the second shared object
# result = libfile2.func(foo_func, bar_func, ctypes.byref(my_struct))
# print("Result:", result)

int_controller.set_log_func(callback)
int_controller.io_write(0xA0, 19, 1)
result = int_controller.io_read(0xA0, 1)
print(result)
int_controller.module_save()
int_controller.module_reset()
result = int_controller.io_read(0xA0, 1)
print("After reset: ", result)
int_controller.module_restore()
result = int_controller.io_read(0xA0, 1)
print("After restore: ", result)