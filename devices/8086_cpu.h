#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "utils.h"

typedef enum {
    invalid_register = 0,
    AX_register,
    AL_register,
    AH_register,
    BX_register,
    BL_register,
    BH_register,
    CX_register,
    CL_register,
    CH_register,
    DX_register,
    DL_register,
    DH_register,
    SI_register,
    DI_register,
    BP_register,
    SP_register,
    IP_register,
    CS_register,
    DS_register,
    ES_register,
    SS_register,
    FLAGS_register,
    override_segment,
} register_name_t;

typedef struct {
    union {
        register_name_t register_name;
        uint32_t address;
    } src;
    union {
        register_name_t register_name;
        uint32_t address;
    } dst;
    int16_t src_val;
    int16_t dst_val;
    uint8_t width;      // operand width in bytes: 1 or 2
    uint8_t src_type;   // 0 - register, 1 - memory
    uint8_t dst_type;   // 0 - register, 1 - memory
    uint8_t num_bytes;  // How many bytes does instruction take
    char * source;
    char * destination;
} operands_t;

// int init_cpu(uint8_t continue_simulation);
// int cpu_tick(void);
// void cpu_save_state(void);
void set_int_vector(uint8_t vector);
// void set_delayed_int(uint8_t vector, uint16_t dely_ticks);
operands_t decode_operands(uint8_t opcode, uint8_t *data, uint8_t single);

// API functions:
void connect_address_space(uint8_t space_type, WRITE_FUNC_PTR(write_func), READ_FUNC_PTR(read_func));
void set_code_read_func(READ_FUNC_PTR(read_func));
void module_reset(void);
void module_save(void);
void module_restore(void);
int module_tick(void);
uint32_t cpu_get_ticks(void);
