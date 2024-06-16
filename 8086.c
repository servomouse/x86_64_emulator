// https://en.wikipedia.org/wiki/Intel_8086
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

struct {
    // Main registers
    uint16_t AX;    // AH=AX>>8; AL=AX&0xFF
    uint16_t BX;    // BH=BX>>8; BL=BX&0xFF
    uint16_t CX;    // CH=CX>>8; CL=CX&0xFF
    uint16_t DX;    // DH=DX>>8; DL=DX&0xFF
    // Index registers
    uint16_t SI;    // Source index
    uint16_t DI;    // Destination index
    uint16_t BP;    // Base pointer
    uint16_t SP;    // Stack pointer
    // Program counter
    uint32_t IP;    // Instruction pointer
    // Segment registers
    uint16_t CS;    // Sode segment
    uint16_t DS;    // Data segment
    uint16_t ES;    // Extra segment
    uint16_t SS;    // Stack segment
    // Status register
    struct{
        unsigned int U3 : 4;    // Unused fields
        unsigned int OF : 1;    // Overflow flag
        unsigned int DF : 1;    // Direction flag
        unsigned int IF : 1;    // Interrupt flag
        unsigned int TF : 1;    // Trap flag
        unsigned int SF : 1;    // Sign flag
        unsigned int ZF : 1;    // Zero flag
        unsigned int U2 : 1;    // Unused field
        unsigned int AF : 1;    // Auxiliary carry flag
        unsigned int U1 : 1;    // Unused field
        unsigned int PF : 1;    // Parity flag
        unsigned int U0 : 1;    // Unused field
        unsigned int CF : 1;    // Carry flag
    } Flags;
} registers;

uint32_t get_addr(uint16_t segment_reg, uint16_t addr) {
    uint32_t ret_val = segment_reg;
    ret_val <<= 4;
    return ret_val + addr;
}

