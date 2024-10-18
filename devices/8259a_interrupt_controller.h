#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


uint8_t int_controller_reset(void);
void int_controller_write(uint32_t addr, uint16_t value, uint8_t width);
uint16_t int_controller_read(uint32_t addr, uint8_t width);