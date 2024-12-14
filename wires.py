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
            print(f"Checking {conn['device_name']}.{conn['pin_name']} state")
            if new_state != conn['struct'].get_state():
                print(f"Setting {conn['device_name']}.{conn['pin_name']} to {new_state}")
                conn['struct']._set_value(new_state)
                conn["struct"].wire_state_change_cb(new_state)
    
    def connect_device(self, device, pin_name, dev_name):
        @ctypes.CFUNCTYPE(ctypes.c_uint8)
        def _wire_get_state():
            return self.state

        @ctypes.CFUNCTYPE(None, ctypes.c_uint8)
        def _wire_set_state(new_state):
            self.set_state(new_state)

        # typedef struct pin_t {
        #     uint8_t state;
        #     pin_type_t pin_type;
        #     uint8_t(*get_state)(void);
        #     uint8_t(*wire_get_state)(void);
        #     void(*set_state)(uint8_t);
        #     void(*wire_set_state)(uint8_t);
        #     void(*wire_state_change_cb)(uint8_t);  // User defined
        # } pin_t;

        class WireStruct(ctypes.Structure):
            _fields_ = [
                ('state', ctypes.c_uint8),
                ("pin_type", WireType),
                ("get_state", ctypes.CFUNCTYPE(ctypes.c_uint8)),
                ("wire_get_state", ctypes.CFUNCTYPE(ctypes.c_uint8)),
                ("_set_value", ctypes.CFUNCTYPE(None, ctypes.c_uint8)),
                ("set_state", ctypes.CFUNCTYPE(None, ctypes.c_uint8)),
                ("wire_set_state", ctypes.CFUNCTYPE(None, ctypes.c_uint8)),
                ("wire_state_change_cb", ctypes.CFUNCTYPE(None, ctypes.c_uint8))
            ]

        wire_struct = WireStruct.in_dll(device, pin_name)
        wire_struct.wire_get_state = _wire_get_state
        wire_struct.wire_set_state = _wire_set_state
        pin = {
            "device_name": dev_name,
            "pin_name": pin_name,
            "struct": wire_struct
        }
        self.connections.append(pin)