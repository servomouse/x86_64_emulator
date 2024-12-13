#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "utils.h"

typedef enum {
    PIN_INPUT = 0,
    PIN_OUTPUT_PP,
    PIN_OUTPUT_OC,
} pin_type_t;

void dummy_cb(uint8_t new_state){}

typedef struct pin_t {
    uint8_t state;
    pin_type_t pin_type;
    uint8_t(*get_state)(void);
    uint8_t(*wire_get_state)(void);
    void(*set_state)(uint8_t);
    void(*wire_set_state)(uint8_t);
    void(*wire_state_change_cb)(uint8_t);  // User defined
} pin_t;

// Create pin with custom callback
#define CREATE_PIN_3(_pin_name, _pin_type, _cb) \
void _pin_name##_set_state(uint8_t new_state);  \
uint8_t _pin_name##_get_state(void);            \
DLL_PREFIX                                      \
pin_t _pin_name = {                             \
    .state = 0,                                 \
    .pin_type = _pin_type,                      \
    .get_state = _pin_name##_get_state,         \
    .wire_get_state = NULL,                     \
    .set_state = _pin_name##_set_state,         \
    .wire_set_state = NULL,                     \
    .wire_state_change_cb = _cb                  \
};                                              \
void _pin_name##_set_state(uint8_t new_state) { \
    if(_pin_name.wire_set_state) {              \
        _pin_name.wire_set_state(new_state);    \
    } else {                                    \
        _pin_name.state = new_state;            \
    }                                           \
}                                               \
uint8_t _pin_name##_get_state(void) {           \
    if(_pin_name.wire_get_state) {              \
        return _pin_name.wire_get_state();      \
    } else {                                    \
        return _pin_name.state;                 \
    }                                           \
}

// Create pin with default (dummy) callback
#define CREATE_PIN_2(_pin_name, _pin_type)      \
void _pin_name##_set_state(uint8_t new_state);  \
uint8_t _pin_name##_get_state(void);            \
DLL_PREFIX                                      \
pin_t _pin_name = {                             \
    .state = 0,                                 \
    .pin_type = _pin_type,                      \
    .get_state = _pin_name##_get_state,         \
    .wire_get_state = NULL,                     \
    .set_state = _pin_name##_set_state,         \
    .wire_set_state = NULL,                     \
    .wire_state_change_cb = dummy_cb             \
};                                              \
void _pin_name##_set_state(uint8_t new_state) { \
    if(_pin_name.wire_set_state) {              \
        _pin_name.wire_set_state(new_state);    \
    } else {                                    \
        _pin_name.state = new_state;            \
    }                                           \
}                                               \
uint8_t _pin_name##_get_state(void) {           \
    if(_pin_name.wire_get_state) {              \
        return _pin_name.wire_get_state();      \
    } else {                                    \
        return _pin_name.state;                 \
    }                                           \
}

// https://gustedt.wordpress.com/2010/06/03/default-arguments-for-c99/
#define _GET_ARG_COUNT(_0, _1, _2, _3, ...) _3
#define GET_ARG_COUNT(...) _GET_ARG_COUNT(__VA_ARGS__, 3, 2, 1, 0)

// Usage:
// CREATE_PIN(int6_pin, PIN_OUTPUT_PP, &custom_callback);
// CREATE_PIN(int7_pin, PIN_OUTPUT_PP);
#define __CREATE_PIN(count, ...) CREATE_PIN_ ## count (__VA_ARGS__)
#define _CREATE_PIN(N, ...) __CREATE_PIN(N, __VA_ARGS__)
#define CREATE_PIN(...) _CREATE_PIN(GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)