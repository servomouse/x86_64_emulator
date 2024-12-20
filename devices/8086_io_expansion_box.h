#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


void module_reset(void);
void data_write(uint32_t addr, uint16_t value, uint8_t width);
uint16_t data_read(uint32_t addr, uint8_t width);
void module_save(void);
void module_restore(void);
int module_tick(uint32_t ticks);
