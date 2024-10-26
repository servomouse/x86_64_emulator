#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "utils.h"

int store_io(void);
int get_io_error(void);     // Returns 1 if there is an IO error, otherwise returns 0

// API functions:
uint32_t map_device(uint32_t start_addr, uint32_t end_addr, WRITE_FUNC_PTR(write_func), READ_FUNC_PTR(read_func));
void unmap_device(uint32_t id);
void data_write(uint32_t addr, uint16_t value, uint8_t width);
uint16_t data_read(uint32_t addr, uint8_t width);
void module_reset(void);
void module_save(void);
void module_restore(void);
int module_tick(void);