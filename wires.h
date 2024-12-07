#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum {
    PIN_INPUT = 0,
    PIN_OUTPUT_PP,
    PIN_OUTPUT_OC,
} pin_type_t;

// typedef enum {
//     PIN_LOW = 0,
//     PIN_HIGH,
// } pin_state_t;

void dummy_cb(uint8_t new_state) {}

void wire_set_state(uint8_t new_state);

typedef struct pin_t {
    uint8_t state;
    pin_type_t pin_type;
    uint8_t(*get_state)(pin_t*);
    uint8_t(*wire_get_state)(void);
    void(*set_state)(pin_t*, uint8_t);
    void(*wire_set_state)(uint8_t);
    void(*pin_state_change_cb)(uint8_t);  // User defined
} pin_t;

// https://gustedt.wordpress.com/2010/06/03/default-arguments-for-c99/

// Example: wire_t wire_name = WIRE_T(WIRE_OUTPUT_PP, &dummy_cb);
// #define WIRE_T(_wire_type, _cb) {.wire_type = _wire_type, .wire_get_state = NULL, .wire_set_state = NULL, .wire_state_change_cb = _cb}
#define CREATE_PIN(_pin_type, _cb) { \
    .pin_type = _pin_type, \
    .pin_get_state = NULL, \
    .pin_set_state = NULL, \
    .pin_state_change_cb = _cb \
    }
