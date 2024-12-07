#include "wires.h"

void dummy_cb(void) {
    return;
}

void pin_set_state(pin_t *pin, uint8_t new_state) {
    if(pin->wire_set_state) {
        pin->wire_set_state(new_state);
    } else {
        pin->state = new_state;
    }
}

uint8_t pin_get_state(pin_t *pin) {
    if(pin->wire_get_state) {
        return pin->wire_get_state();
    } else {
        return pin->state;
    }
}
