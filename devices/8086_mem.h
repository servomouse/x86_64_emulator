#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "utils.h"


uint8_t * mem_init(uint8_t continue_simulation);
void data_write(uint32_t addr, uint16_t value, uint8_t width);
uint16_t data_read(uint32_t addr, uint8_t width);
uint16_t code_read(uint32_t addr, uint8_t width);
int store_memory(void);

void module_reset(void);
void module_save(void);
void module_restore(void);
uint32_t map_device(uint32_t start_addr, uint32_t end_addr, WRITE_FUNC_PTR(write_func), READ_FUNC_PTR(read_func));
int module_tick(uint32_t ticks);