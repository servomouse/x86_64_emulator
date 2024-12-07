import ctypes

class WireType(ctypes.c_int):
    WIRE_INPUT = 0
    WIRE_OUTPUT_PP = 1
    WIRE_OUTPUT_OC = 2


class Wire:
    def __init__(self, name):
        self.name = name
        self.state = 0
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
        @ctypes.CFUNCTYPE(ctypes.c_uint8)
        def _wire_get_state():
            return self.state

        @ctypes.CFUNCTYPE(None, ctypes.c_uint8)
        def _wire_set_state(new_state):
            self.set_state(new_state.value)

        class WireStruct(ctypes.Structure):
            _fields_ = [("wire_type", WireType),
                        ("wire_get_state", ctypes.CFUNCTYPE(ctypes.c_uint8)),
                        ("wire_set_state", ctypes.CFUNCTYPE(None, ctypes.c_uint8)),
                        ("wire_state_change_cb", ctypes.CFUNCTYPE(None, ctypes.c_uint8))]

        wire_struct = WireStruct.in_dll(device, self.name)
        wire_struct.wire_get_state = _wire_get_state
        wire_struct.wire_set_state = _wire_set_state
        self.connections.append(wire_struct)