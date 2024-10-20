#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "utils.h"

void module_reset(void);
void io_write(uint32_t addr, uint16_t value, uint8_t width);
uint16_t io_read(uint32_t addr, uint8_t width);
void module_save(void);
void module_restore(void);