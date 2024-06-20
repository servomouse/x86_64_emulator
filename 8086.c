// https://en.wikipedia.org/wiki/Intel_8086
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

struct {
    // Main registers
    uint16_t AX;    // Accumulator AH=AX>>8; AL=AX&0xFF
    uint16_t BX;    // Base BH=BX>>8; BL=BX&0xFF
    uint16_t CX;    // Count CH=CX>>8; CL=CX&0xFF
    uint16_t DX;    // Data DH=DX>>8; DL=DX&0xFF
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
    } flags;
} registers;

typedef enum {
    AX = 0,
    BX,
    CX,
    DX,
    SI,
    DI,
    BP,
    SP,
    IP,
    CS,
    DS,
    ES,
    SS,
} register_t;

/*===== J-instructions =====*/
// https://www.dei.isep.ipp.pt/~nsilva/ensino/ArqC/ArqC1998-1999/nguide/ng-j.htm

uint8_t jo_op(void) {   // jump if the overflow flag is set
    return 1;
}

uint8_t jb_op(void) {   // Jump if Carry
    return 1;
}

uint8_t je_op(void) {   // Same as JZ: Jump if SF = 1
    return 1;
}

uint8_t jbe_op(void) {  // Jump if CF = 1 or ZF = 1
    return 1;
}

uint8_t js_op(void) {   // Jump if SF = 1
    return 1;
}

uint8_t jp_op(void) {   // Jump if PF = 1
    return 1;
}

uint8_t jl_op(void) {   // Jump if SF <> OF (jump if less)
    return 1;
}

uint8_t jle_op(void) {  // Jump if SF <> OF or ZF = 1 (jump if less or equal)
    return 1;
}

/*==== !J-instructions =====*/

uint32_t segment_override(register_t segment) {
    return 1;
}

uint32_t inc_register(register_t reg) {
    return 1;
}

uint32_t get_addr(uint16_t segment_reg, uint16_t addr) {
    uint32_t ret_val = segment_reg;
    ret_val <<= 4;
    return ret_val + addr;
}

#define INVALID_INSTRUCTION while(1)

uint8_t get_mode_field(uint8_t byte) {
    return byte >> 6;
}

uint8_t get_register_field(uint8_t byte) {
    return (byte & 0xC7) >> 3;
}

uint8_t get_reg_mem_field(uint8_t byte) {
    return byte & 0x07;
}

uint8_t add_op(uint8_t opcode, uint8_t *data) {
    return 1;
}

uint8_t or_op(uint8_t opcode, uint8_t *data) {
    return 1;
}

uint8_t adc_op(uint8_t opcode, uint8_t *data) {
    return 1;
}

uint8_t sbb_op(uint8_t opcode, uint8_t *data) {
    return 1;
}

uint8_t and_op(uint8_t opcode, uint8_t *data) {
    return 1;
}

uint8_t sub_op(uint8_t opcode, uint8_t *data) {
    return 1;
}

uint8_t xor_op(uint8_t opcode, uint8_t *data) {
    return 1;
}

uint8_t cmp_op(uint8_t opcode, uint8_t *data) {
    return 1;
}

uint8_t inc_op(uint8_t opcode, uint8_t *data) {
    return 1;
}

uint8_t dec_op(uint8_t opcode, uint8_t *data) {
    return 1;
}

uint8_t push_reg_op(register_t reg) {
    return 1;
}

uint8_t pop_reg_op(register_t reg) {
    return 1;
}

uint8_t aaa_op(void) {
    return 1;
}

uint8_t aas_op(void) {
    return 1;
}

uint8_t push_val_op(uint16_t val) {
    return 1;
}

void process_instruction(uint8_t *memory) {
    switch(memory[0]) {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
            registers.IP += add_op(memory[1], &memory[2]);
            break;
        case 0x06:
            registers.IP += push_reg_op(registers.ES);
            break;
        case 0x07:
            registers.IP += pop_reg_op(ES);
            break;
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
            registers.IP += or_op(memory[1], &memory[2]);
            break;
        case 0x0E:
            registers.IP += push_reg_op(registers.CS);
            break;
        case 0x0F:
            INVALID_INSTRUCTION;
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15:
            registers.IP += adc_op(memory[1], &memory[2]);
            break;
        case 0x16:
            registers.IP += push_reg_op(registers.SS);
            break;
        case 0x17:
            registers.IP += pop_reg_op(SS);
            break;
        case 0x18:
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
            registers.IP += sbb_op(memory[1], &memory[2]);
            break;
        case 0x1E:
            registers.IP += push_reg_op(registers.DS);
            break;
        case 0x1F:
            registers.IP += pop_reg_op(DS);
            break;
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x25:
            registers.IP += and_op(memory[1], &memory[2]);
            break;
        case 0x26:
            registers.IP += segment_override(ES);  // ES: segment overrige prefix
            break;
        case 0x27:
            registers.IP += daa_op();
            break;
        case 0x28:
        case 0x29:
        case 0x2A:
        case 0x2B:
        case 0x2C:
        case 0x2D:
            registers.IP += sub_op(memory[1], &memory[2]);
            break;
        case 0x2E:
            registers.IP += segment_override(CS);  // CS: segment overrige prefix
            break;
        case 0x2F:
            registers.IP += das_op();
            break;
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
            registers.IP += xor_op(memory[1], &memory[2]);
            break;
        case 0x36:
            registers.IP += segment_override(SS);  // SS: segment overrige prefix
            break;
        case 0x37:
            registers.IP += aaa_op();
            break;
        case 0x38:
        case 0x39:
        case 0x3A:
        case 0x3B:
        case 0x3C:
        case 0x3D:
            registers.IP += cmp_op(memory[1], &memory[2]);
            break;
        case 0x3E:
            registers.IP += segment_override(DS);  // DS: segment overrige prefix
            break;
        case 0x3F:
            registers.IP += aas_op();
            break;
        case 0x40:
            registers.IP += inc_register(AX);
            break;
        case 0x41:
            registers.IP += inc_register(CX);
            break;
        case 0x42:
            registers.IP += inc_register(DX);
            break;
        case 0x43:
            registers.IP += inc_register(BX);
            break;
        case 0x44:
            registers.IP += inc_register(SP);
            break;
        case 0x45:
            registers.IP += inc_register(BP);
            break;
        case 0x46:
            registers.IP += inc_register(SI);
            break;
        case 0x47:
            registers.IP += inc_register(DI);
            break;
        case 0x48:
            registers.IP += dec_register(AX);
            break;
        case 0x49:
            registers.IP += dec_register(CX);
            break;
        case 0x4A:
            registers.IP += dec_register(DX);
            break;
        case 0x4B:
            registers.IP += dec_register(BX);
            break;
        case 0x4C:
            registers.IP += dec_register(SP);
            break;
        case 0x4D:
            registers.IP += dec_register(BP);
            break;
        case 0x4E:
            registers.IP += dec_register(SI);
            break;
        case 0x4F:
            registers.IP += dec_register(DI);
            break;
        case 0x50:
            registers.IP += push_reg_op(AX);
            break;
        case 0x51:
            registers.IP += push_reg_op(CX);
            break;
        case 0x52:
            registers.IP += push_reg_op(DX);
            break;
        case 0x53:
            registers.IP += push_reg_op(BX);
            break;
        case 0x54:
            registers.IP += push_reg_op(SP);
            break;
        case 0x55:
            registers.IP += push_reg_op(BP);
            break;
        case 0x56:
            registers.IP += push_reg_op(SI);
            break;
        case 0x57:
            registers.IP += push_reg_op(DI);
            break;
        case 0x58:
            registers.IP += pop_reg_op(AX);
            break;
        case 0x59:
            registers.IP += pop_reg_op(CX);
            break;
        case 0x5A:
            registers.IP += pop_reg_op(DX);
            break;
        case 0x5B:
            registers.IP += pop_reg_op(BX);
            break;
        case 0x5C:
            registers.IP += pop_reg_op(SP);
            break;
        case 0x5D:
            registers.IP += pop_reg_op(BP);
            break;
        case 0x5E:
            registers.IP += pop_reg_op(SI);
            break;
        case 0x5F:
            registers.IP += pop_reg_op(DI);
            break;
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
        case 0x68:
        case 0x69:
        case 0x6A:
        case 0x6B:
        case 0x6C:
        case 0x6D:
        case 0x6E:
        case 0x6F:
            INVALID_INSTRUCTION;
            break;
        case 0x70:
            if(jo_op())
                registers.IP = memory[1];
            break;
        case 0x71:
            if(!jo_op())
                registers.IP = memory[1];
            break;
        case 0x72:
            if(jb_op())
                registers.IP = memory[1];
            break;
        case 0x73:
            if(!jb_op())
                registers.IP = memory[1];
            break;
        case 0x74:
            if(je_op())
                registers.IP = memory[1];
            break;
        case 0x75:
            if(!je_op())
                registers.IP = memory[1];
            break;
        case 0x76:
            if(jbe_op())
                registers.IP = memory[1];
            break;
        case 0x77:
            if(!jbe_op())
                registers.IP = memory[1];
            break;
        case 0x78:
            if(js_op())
                registers.IP = memory[1];
            break;
        case 0x79:
            if(!js_op())
                registers.IP = memory[1];
            break;
        case 0x7A:
            if(jp_op())
                registers.IP = memory[1];
            break;
        case 0x7B:
            if(!jp_op())
                registers.IP = memory[1];
            break;
        case 0x7C:
            if(jl_op())
                registers.IP = memory[1];
            break;
        case 0x7D:
            if(!jl_op())
                registers.IP = memory[1];
            break;
        case 0x7E:
            if(jle_op())
                registers.IP = memory[1];
            break;
        case 0x7F:
            if(!jle_op())
                registers.IP = memory[1];
            break;
        case 0x80:
            // 8-bit operations
            switch(get_mode_field(memory[1])) {
                case 0:
                    registers.IP += add_op(memory[1], &memory[2]);
                    break;
                case 1:
                    registers.IP += or_op(memory[1], &memory[2]);
                    break;
                case 2:
                    registers.IP += adc_op(memory[1], &memory[2]);
                    break;
                case 3:
                    registers.IP += sbb_op(memory[1], &memory[2]);
                    break;
                case 4:
                    registers.IP += and_op(memory[1], &memory[2]);
                    break;
                case 5:
                    registers.IP += sub_op(memory[1], &memory[2]);
                    break;
                case 6:
                    registers.IP += xor_op(memory[1], &memory[2]);
                    break;
                case 7:
                    registers.IP += cmp_op(memory[1], &memory[2]);
                    break;
            }
            break;
        case 0x81:
            // 16-bit operations
            switch(get_mode_field(memory[1])) {
                case 0:
                    registers.IP += add_op(memory[1], &memory[2]);
                    break;
                case 1:
                    registers.IP += or_op(memory[1], &memory[2]);
                    break;
                case 2:
                    registers.IP += adc_op(memory[1], &memory[2]);
                    break;
                case 3:
                    registers.IP += sbb_op(memory[1], &memory[2]);
                    break;
                case 4:
                    registers.IP += and_op(memory[1], &memory[2]);
                    break;
                case 5:
                    registers.IP += sub_op(memory[1], &memory[2]);
                    break;
                case 6:
                    registers.IP += xor_op(memory[1], &memory[2]);
                    break;
                case 7:
                    registers.IP += cmp_op(memory[1], &memory[2]);
                    break;
            }
            break;
        case 0x82:
            // 8-bit operations
            switch(get_mode_field(memory[1])) {
                case 0:
                    registers.IP += add_op(memory[1], &memory[2]);
                    break;
                case 1:
                    INVALID_INSTRUCTION;
                    break;
                case 2:
                    registers.IP += adc_op(memory[1], &memory[2]);
                    break;
                case 3:
                    registers.IP += sbb_op(memory[1], &memory[2]);
                    break;
                case 4:
                    INVALID_INSTRUCTION;
                    break;
                case 5:
                    registers.IP += sub_op(memory[1], &memory[2]);
                    break;
                case 6:
                    INVALID_INSTRUCTION;
                    break;
                case 7:
                    registers.IP += cmp_op(memory[1], &memory[2]);
                    break;
            }
            break;
        case 0x83:
            // 16-bit operations
            switch(get_mode_field(memory[1])) {
                case 0:
                    registers.IP += add_op(memory[1], &memory[2]);
                    break;
                case 1:
                    INVALID_INSTRUCTION;
                    break;
                case 2:
                    registers.IP += adc_op(memory[1], &memory[2]);
                    break;
                case 3:
                    registers.IP += sbb_op(memory[1], &memory[2]);
                    break;
                case 4:
                    INVALID_INSTRUCTION;
                    break;
                case 5:
                    registers.IP += sub_op(memory[1], &memory[2]);
                    break;
                case 6:
                    INVALID_INSTRUCTION;
                    break;
                case 7:
                    registers.IP += cmp_op(memory[1], &memory[2]);
                    break;
            }
            break;
        case 0x84:
            break;
        case 0x85:
            break;
        case 0x86:
            break;
        case 0x87:
            break;
        case 0x88:
            break;
        case 0x89:
            break;
        case 0x8A:
            break;
        case 0x8B:
            break;
        case 0x8C:
            break;
        case 0x8D:
            break;
        case 0x8E:
            break;
        case 0x8F:
            break;
        case 0x90:
            break;
        case 0x91:
            break;
        case 0x92:
            break;
        case 0x93:
            break;
        case 0x94:
            break;
        case 0x95:
            break;
        case 0x96:
            break;
        case 0x97:
            break;
        case 0x98:
            break;
        case 0x99:
            break;
        case 0x9A:
            break;
        case 0x9B:
            break;
        case 0x9C:
            break;
        case 0x9D:
            break;
        case 0x9E:
            break;
        case 0x9F:
            break;
        case 0xA0:
            break;
        case 0xA1:
            break;
        case 0xA2:
            break;
        case 0xA3:
            break;
        case 0xA4:
            break;
        case 0xA5:
            break;
        case 0xA6:
            break;
        case 0xA7:
            break;
        case 0xA8:
            break;
        case 0xA9:
            break;
        case 0xAA:
            break;
        case 0xAB:
            break;
        case 0xAC:
            break;
        case 0xAD:
            break;
        case 0xAE:
            break;
        case 0xAF:
            break;
        case 0xB0:
            break;
        case 0xB1:
            break;
        case 0xB2:
            break;
        case 0xB3:
            break;
        case 0xB4:
            break;
        case 0xB5:
            break;
        case 0xB6:
            break;
        case 0xB7:
            break;
        case 0xB8:
            break;
        case 0xB9:
            break;
        case 0xBA:
            break;
        case 0xBB:
            break;
        case 0xBC:
            break;
        case 0xBD:
            break;
        case 0xBE:
            break;
        case 0xBF:
            break;
        case 0xC0:
            break;
        case 0xC1:
            break;
        case 0xC2:
            break;
        case 0xC3:
            break;
        case 0xC4:
            break;
        case 0xC5:
            break;
        case 0xC6:
            break;
        case 0xC7:
            break;
        case 0xC8:
            break;
        case 0xC9:
            break;
        case 0xCA:
            break;
        case 0xCB:
            break;
        case 0xCC:
            break;
        case 0xCD:
            break;
        case 0xCE:
            break;
        case 0xCF:
            break;
        case 0xD0:
            break;
        case 0xD1:
            break;
        case 0xD2:
            break;
        case 0xD3:
            break;
        case 0xD4:
            break;
        case 0xD5:
            break;
        case 0xD6:
            break;
        case 0xD7:
            break;
        case 0xD8:
            break;
        case 0xD9:
            break;
        case 0xDA:
            break;
        case 0xDB:
            break;
        case 0xDC:
            break;
        case 0xDD:
            break;
        case 0xDE:
            break;
        case 0xDF:
            break;
        case 0xE0:
            break;
        case 0xE1:
            break;
        case 0xE2:
            break;
        case 0xE3:
            break;
        case 0xE4:
            break;
        case 0xE5:
            break;
        case 0xE6:
            break;
        case 0xE7:
            break;
        case 0xE8:
            break;
        case 0xE9:
            break;
        case 0xEA:
            break;
        case 0xEB:
            break;
        case 0xEC:
            break;
        case 0xED:
            break;
        case 0xEE:
            break;
        case 0xEF:
            break;
        case 0xF0:
            break;
        case 0xF1:
            break;
        case 0xF2:
            break;
        case 0xF3:
            break;
        case 0xF4:
            break;
        case 0xF5:
            break;
        case 0xF6:
            break;
        case 0xF7:
            break;
        case 0xF8:
            break;
        case 0xF9:
            break;
        case 0xFA:
            break;
        case 0xFB:
            break;
        case 0xFC:
            break;
        case 0xFD:
            break;
        case 0xFE:
            break;
        case 0xFF:
            break;

        default:
            INVALID_INSTRUCTION;
    }
}
