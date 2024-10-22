#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum {
    WIRE_INPUT = 0,
    WIRE_OUTPUT_PP,
    WIRE_OUTPUT_OC,
} wire_type_t;

typedef enum {
    WIRE_LOW = 0,
    WIRE_HIGH,
} wire_state_t;

typedef struct wire_t {
    const wire_type_t wire_type;
    wire_state_t(*wire_get_state)(void);
    void(*wire_set_state)(wire_state_t);
    void(*wire_state_change_cb)(wire_state_t);  // User defined
} wire_t;

#define WIRE_T(_wire_type, _cb) {.wire_type = _wire_type, .wire_get_state = NULL, .wire_set_state = NULL, .wire_state_change_cb = _cb}
