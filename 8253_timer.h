#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


uint8_t timer_init(void);
uint8_t timer_write(uint32_t addr, uint16_t value, uint8_t width);
uint16_t timer_read(uint32_t addr, uint8_t width);
void timer_set_output_cb(uint8_t idx, void (*fun_ptr)(uint8_t));
void timer_set_gate(uint8_t idx, uint8_t value);
void timer_tick(void);
