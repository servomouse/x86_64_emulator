#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


uint8_t mda_init(void);
uint8_t mda_write(uint32_t addr, uint16_t value, uint8_t width);
uint16_t mda_read(uint32_t addr, uint8_t width);
