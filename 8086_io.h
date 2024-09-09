#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

void io_write(uint32_t addr, uint16_t value, uint8_t width);
uint16_t io_read(uint32_t addr, uint8_t width);
int io_init(void);