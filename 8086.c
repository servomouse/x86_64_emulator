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

uint16_t set_h(uint16_t reg_value, uint8_t value) {
    reg_value &= 0xFF;
    reg_value |= (value << 8);
    return reg_value;
}

uint16_t set_l(uint16_t reg_value, uint8_t value) {
    reg_value &= 0xFF00;
    reg_value |= value;
    return reg_value;
}

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

uint8_t test_8b_op(uint8_t opcode, uint8_t *data) {
    return 1;
}

uint8_t test_16b_op(uint8_t opcode, uint8_t *data) {
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
            registers.IP += test_8b_op(memory[1], &memory[2]);
            break;
        case 0x85:
            registers.IP += test_16b_op(memory[1], &memory[2]);
            break;
        case 0x86:
            registers.IP += xchg_8b_op(memory[1], &memory[2]);
            break;
        case 0x87:
            registers.IP += xchg_16b_op(memory[1], &memory[2]);
            break;
        case 0x88:
            registers.IP += mov_16b_op(memory[1], &memory[2]);
            break;
        case 0x89:
            registers.IP += mov_16b_op(memory[1], &memory[2]);
            break;
        case 0x8A:
            registers.IP += mov_16b_op(memory[1], &memory[2]);
            break;
        case 0x8B:
            registers.IP += mov_16b_op(memory[1], &memory[2]);
            break;
        case 0x8C:
            if(3 & get_mode_field(memory[1]))
                INVALID_INSTRUCTION;
            else
                registers.IP += mov_16b_op(memory[1], &memory[2]);
            break;
        case 0x8D:  // LEA REG16, MEM16
            registers.IP += lea_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
            break;
        case 0x8E:
            if((3 & get_mode_field(memory[1])) == 0)    // MOV SEGREG, REG16/MEM16
                registers.IP += mov_16b_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
            else
                INVALID_INSTRUCTION;
            break;
        case 0x8F:
            if(0 == get_mode_field(memory[1]))  // POP REG16/MEM16
                registers.IP += pop_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
            else
                INVALID_INSTRUCTION;
            break;
        case 0x90:  // NOP, XCHG AX AX
                registers.IP += nop_op();
            break;
        case 0x91:  // XCHG AX CX
                registers.IP += xchg_reg_op(AX, CX);
            break;
        case 0x92:  // XCHG AX, DX
                registers.IP += xchg_reg_op(AX, DX);
            break;
        case 0x93:  // XCHG AX, BX
                registers.IP += xchg_reg_op(AX, BX);
            break;
        case 0x94:  // XCHG AX, SP
                registers.IP += xchg_reg_op(AX, SP);
            break;
        case 0x95:  // XCHG AX, BP
                registers.IP += xchg_reg_op(AX, BP);
            break;
        case 0x96:  // XCHG AX, SI
                registers.IP += xchg_reg_op(AX, SI);
            break;
        case 0x97:  // XCHG AX, DI
                registers.IP += xchg_reg_op(AX, DI);
            break;
        case 0x98:  // CBW
                registers.IP += cbw_op();
            break;
        case 0x99:  // CWD
                registers.IP += cwd_op();
            break;
        case 0x9A:  // CALL FAR_PROC
            registers.IP += call_op(memory[1], &memory[2]); // [DISP-LO, DISP-HIGH, SEG-LO, SEG-HI]
            break;
        case 0x9B:  // WAIT
                registers.IP += wait_op();
            break;
        case 0x9C:  // PUSHF
                registers.IP += pushf_op();
            break;
        case 0x9D:  // POPF
                registers.IP += popf_op();
            break;
        case 0x9E:  // SAHF
                registers.IP += sahf_op();
            break;
        case 0x9F:  // LAHF
                registers.IP += lahf_op();
            break;
        case 0xA0:  // MOV AL, MEM8
            registers.IP += mov_op(memory[1], &memory[2]); // ADDR-LO, ADDR-HI
            break;
        case 0xA1:  // MOV AX, MEM16
            registers.IP += mov_op(memory[1], &memory[2]); // ADDR-LO, ADDR-HI
            break;
        case 0xA2:  // MOV MEM8, AL
            registers.IP += mov_op(memory[1], &memory[2]); // ADDR-LO, ADDR-HI
            break;
        case 0xA3:  // MOV MEM8, AX
            registers.IP += mov_op(memory[1], &memory[2]); // ADDR-LO, ADDR-HI
            break;
        case 0xA4:  // MOVS DEST-STR8, SRC-STR8
            registers.IP += mov_str_op(memory[1], &memory[2]);
            break;
        case 0xA5:  // MOVS DEST-STR16, SRC-STR16
            registers.IP += mov_str_op(memory[1], &memory[2]);
            break;
        case 0xA6:  // CMPSS DEST-STR8, SRC-STR8
            registers.IP += cmps_str_op(memory[1], &memory[2]);
            break;
        case 0xA7:  // CMPSS DEST-STR16, SRC-STR16
            registers.IP += cmps_str_op(memory[1], &memory[2]);
            break;
        case 0xA8:  // TEST AL, IMMED8
            registers.IP += cmps_str_op(memory[1], &memory[2]); // DATA8
            break;
        case 0xA9:  // TEST AX, IMMED16
            registers.IP += cmps_str_op(memory[1], &memory[2]); // DATA_LO, DATA-HI
            break;
        case 0xAA:  // STOS DEST-STR8
            registers.IP += stos_str_op(memory[1], &memory[2]);
            break;
        case 0xAB:  // STOS DEST-STR16
            registers.IP += stos_str_op(memory[1], &memory[2]);
            break;
        case 0xAC:  // LODS DEST-STR8
            registers.IP += stos_str_op(memory[1], &memory[2]);
            break;
        case 0xAD:  // LODS DEST-STR16
            registers.IP += stos_str_op(memory[1], &memory[2]);
            break;
        case 0xAE:  // SCAS DEST-STR8
            registers.IP += stos_str_op(memory[1], &memory[2]);
            break;
        case 0xAF:  // SCAS DEST-STR16
            registers.IP += stos_str_op(memory[1], &memory[2]);
            break;
        case 0xB0:  // MOV AL, IMMED8
            registers.AX = set_l(registers.AX, memory[1]); // DATA-8
            registers.IP += 2;
            break;
        case 0xB1:  // MOV CL, IMMED8
            registers.CX = set_l(registers.CX, memory[1]); // DATA-8
            registers.IP += 2;
            break;
        case 0xB2:  // MOV DL, IMMED8
            registers.DX = set_l(registers.DX, memory[1]); // DATA-8
            registers.IP += 2;
            break;
        case 0xB3:  // MOV BL, IMMED8
            registers.BX = set_l(registers.BX, memory[1]); // DATA-8
            registers.IP += 2;
            break;
        case 0xB4:  // MOV AH, IMMED8
            registers.AX = set_h(registers.AX, memory[1]); // DATA-8
            registers.IP += 2;
            break;
        case 0xB5:  // MOV CH, IMMED8
            registers.CX = set_h(registers.CX, memory[1]); // DATA-8
            registers.IP += 2;
            break;
        case 0xB6:  // MOV DH, IMMED8
            registers.DX = set_h(registers.DX, memory[1]); // DATA-8
            registers.IP += 2;
            break;
        case 0xB7:  // MOV BH, IMMED8
            registers.BX = set_h(registers.BX, memory[1]); // DATA-8
            registers.IP += 2;
            break;
        case 0xB8:  // MOV AX, IMMED16
            registers.AX = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
            registers.IP += 3;
            break;
        case 0xB9:  // MOV CX, IMMED16
            registers.CX = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
            registers.IP += 3;
            break;
        case 0xBA:  // MOV DX, IMMED16
            registers.DX = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
            registers.IP += 3;
            break;
        case 0xBB:  // MOV BX, IMMED16
            registers.BX = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
            registers.IP += 3;
            break;
        case 0xBC:  // MOV SP, IMMED16
            registers.SP = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
            registers.IP += 3;
            break;
        case 0xBD:  // MOV BP, IMMED16
            registers.BP = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
            registers.IP += 3;
            break;
        case 0xBE:  // MOV SI, IMMED16
            registers.SI = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
            registers.IP += 3;
            break;
        case 0xBF:  // MOV DI, IMMED16
            registers.DI = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
            registers.IP += 3;
            break;
        case 0xC0:
            INVALID_INSTRUCTION;
            break;
        case 0xC1:
            INVALID_INSTRUCTION;
            break;
        case 0xC2:  // RET IMMED16 (intrasegment)
            registers.IP += ret_op(memory[1], &memory[2]);  // DATA-LO, DATA-HI
            break;
        case 0xC3:  // RET (intrasegment)
            registers.IP += ret_op(memory[1], &memory[2]);
            break;
        case 0xC4:  // LES REG16, MEM16
            registers.IP += les_op(memory[1], &memory[2]);  // MOD REG R/M, DISP-LO, DISP-HI
            break;
        case 0xC5:  // LDS REG16, MEM16
            registers.IP += lds_op(memory[1], &memory[2]);  // MOD REG R/M, DISP-LO, DISP-HI
            break;
        case 0xC6:  // MOV MEM8, IMMED8
            if(get_mode_field(memory[1]) == 0) {
                registers.IP += mov_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI, DATA-8
            } else {
                INVALID_INSTRUCTION;
            }
            break;
        case 0xC7:  // MOV MEM16, IMMED16
            if(get_mode_field(memory[1]) == 0) {
                registers.IP += mov_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI, DATA-LO, DATA-HI
            } else {
                INVALID_INSTRUCTION;
            }
            break;
        case 0xC8:
            INVALID_INSTRUCTION;
            break;
        case 0xC9:
            INVALID_INSTRUCTION;
            break;
        case 0xCA:  // RET IMMED16 (intersegment)
            registers.IP += ret_op(memory[1], &memory[2]);  // DATA-LO, DATA-HI
            break;
        case 0xCB:  // RET (intersegment)
            registers.IP += ret_op(memory[1], &memory[2]);
            break;
        case 0xCC:  // INT 3
            registers.IP += int_op(3);
            break;
        case 0xCD:  // INT IMMED8
            registers.IP += int_op(memory[1]);  // DATA-8
            break;
        case 0xCE:  // INTO
            registers.IP += into_op();
            break;
        case 0xCF:  // IRET
            registers.IP += iret_op();
            break;
        case 0xD0:
            // 8-bit operations
            switch(get_mode_field(memory[1])) {
                case 0: // ROL REG8/MEM8, 1
                    registers.IP += rol_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 1: // ROR REG8/MEM8, 1
                    registers.IP += ror_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 2: // RCL REG8/MEM8, 1
                    registers.IP += rcl_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 3: // RCR REG8/MEM8, 1
                    registers.IP += rcr_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 4: // SAL/SHL REG8/MEM8, 1
                    registers.IP += shl_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 5: // SHR REG8/MEM8, 1
                    registers.IP += shr_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 6:
                    INVALID_INSTRUCTION;
                    break;
                case 7: // SAR REG8/MEM8, 1
                    registers.IP += sar_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
            }
            break;
        case 0xD1:
            // 16-bit operations
            switch(get_mode_field(memory[1])) {
                case 0: // ROL REG16/MEM16, 1
                    registers.IP += rol_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 1: // ROR REG16/MEM16, 1
                    registers.IP += ror_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 2: // RCL REG16/MEM16, 1
                    registers.IP += rcl_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 3: // RCR REG16/MEM16, 1
                    registers.IP += rcr_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 4: // SAL/SHL REG16/MEM16, 1
                    registers.IP += shl_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 5: // SHR REG16/MEM16, 1
                    registers.IP += shr_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 6:
                    INVALID_INSTRUCTION;
                    break;
                case 7: // SAR REG16/MEM16, 1
                    registers.IP += sar_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
            }
            break;
        case 0xD2:
            // 8-bit operations
            switch(get_mode_field(memory[1])) {
                case 0: // ROL REG8/MEM8, CL
                    registers.IP += rol_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 1: // ROR REG8/MEM8, CL
                    registers.IP += ror_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 2: // RCL REG8/MEM8, CL
                    registers.IP += rcl_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 3: // RCR REG8/MEM8, CL
                    registers.IP += rcr_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 4: // SAL/SHL REG8/MEM8, CL
                    registers.IP += shl_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 5: // SHR REG8/MEM8, CL
                    registers.IP += shr_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 6:
                    INVALID_INSTRUCTION;
                    break;
                case 7: // SAR REG8/MEM8, CL
                    registers.IP += sar_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
            }
            break;
        case 0xD3:
            // 16-bit operations
            switch(get_mode_field(memory[1])) {
                case 0: // ROL REG16/MEM16, CL
                    registers.IP += rol_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 1: // ROR REG16/MEM16, CL
                    registers.IP += ror_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 2: // RCL REG16/MEM16, CL
                    registers.IP += rcl_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 3: // RCR REG16/MEM16, CL
                    registers.IP += rcr_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 4: // SAL/SHL REG16/MEM16, CL
                    registers.IP += shl_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 5: // SHR REG16/MEM16, CL
                    registers.IP += shr_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
                case 6:
                    INVALID_INSTRUCTION;
                    break;
                case 7: // SAR REG16/MEM16, CL
                    registers.IP += sar_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
                    break;
            }
            break;
        case 0xD4:
            if(memory[1] == 0b00001010) {  // AAM
                registers.IP += aam_op(memory[1]);  // memory[1] == 0b00001010
            } else {
                INVALID_INSTRUCTION;
            }
            break;
        case 0xD5:
            if(memory[1] == 0b00001010) {  // AAD
                registers.IP += aam_op(memory[1]);  // memory[1] == 0b00001010
            } else {
                INVALID_INSTRUCTION;
            }
            break;
        case 0xD6:
            INVALID_INSTRUCTION;
            break;
        case 0xD7:  // XLAT SOURCE-TABLE
            registers.IP += aam_op();
            break;
        case 0xD8:
        case 0xD9:
        case 0xDA:
        case 0xDB:
        case 0xDC:
        case 0xDD:
        case 0xDE:  // ESC OPCODE, SOURCE
        case 0xDF:
            if((memory[0] == 0xD8 && get_register_field(memory[1]) == 0) ||
               (memory[0] == 0xDF && get_register_field(memory[1]) == 0x07)) {
                registers.IP += esc_op();   // TODO: Not sure
            } else {
                registers.IP += esc_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
            }
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
