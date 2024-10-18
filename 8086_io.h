#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


#define READ_FUNC_PTR  uint16_t (*read_func)(uint32_t, uint8_t)
#define WRITE_FUNC_PTR void (*write_func)(uint32_t, uint16_t, uint8_t)

uint32_t map_device(uint32_t start_addr, uint32_t end_addr, WRITE_FUNC_PTR, READ_FUNC_PTR);
void unmap_device(uint32_t id);

void io_write(uint32_t addr, uint16_t value, uint8_t width);
uint16_t io_read(uint32_t addr, uint8_t width);
int io_init(uint8_t continue_simulation);
int store_io(void);
int get_io_error(void);     // Returns 1 if there is an IO error, otherwise returns 0