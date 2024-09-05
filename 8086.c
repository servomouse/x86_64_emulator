// https://en.wikipedia.org/wiki/Intel_8086
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define elif else if

uint8_t *IO_SPACE = NULL;
uint8_t *MEMORY = NULL;

#define INVALID_INSTRUCTION while(1)

typedef struct {
    // Helper registers
    uint32_t ticks;
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
    // struct{
    //     unsigned int U3 : 4;    // Unused fields
    //     unsigned int OF : 1;    // Overflow flag
    //     unsigned int DF : 1;    // Direction flag, used to influence the direction in which string instructions offset pointer registers
    //     unsigned int IF : 1;    // Interrupt flag
    //     unsigned int TF : 1;    // Trap flag
    //     unsigned int SF : 1;    // Sign flag
    //     unsigned int ZF : 1;    // Zero flag
    //     unsigned int U2 : 1;    // Unused field
    //     unsigned int AF : 1;    // Auxiliary (decimal) carry flag
    //     unsigned int U1 : 1;    // Unused field
    //     unsigned int PF : 1;    // Parity flag
    //     unsigned int U0 : 1;    // Unused field
    //     unsigned int CF : 1;    // Carry flag
    // } flags;
    uint16_t flags;
} registers_t;
registers_t *REGS = NULL;

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
} register_name_t;

typedef enum{
    CF = 0,    // Carry flag
    U0,    // Unused field
    PF,    // Parity flag
    U1,    // Unused field
    AF,    // Auxiliary (decimal) carry flag
    U2,    // Unused field
    ZF,    // Zero flag
    SF,    // Sign flag
    TF,    // Trap flag
    IF,    // Interrupt flag
    DF,    // Direction flag, used to influence the direction in which string instructions offset pointer registers
    OF,    // Overflow flag
    U3,    // Unused fields
} flag_t;

uint8_t get_flag(flag_t flag) {
    return (REGS->flags & (1 << flag)) > 0;
}

void set_flag(flag_t flag, uint16_t value) {
    uint16_t mask = 1 << flag;
    if (value > 0) {
        REGS->flags |= mask;
    } else {
        REGS->flags &= ~mask;
    }
}

uint32_t get_addr(uint16_t segment_reg, uint16_t addr) {
    uint32_t ret_val = segment_reg;
    ret_val <<= 4;
    return ret_val + addr;
}

uint8_t get_mode_field(uint8_t byte) {
    // 00 -> Memory mode, no displacement follows (except when reg_mem_field == 110, then 16-bit displacement follows)
    // 01 -> Memory mode, 8-bit displacement follows
    // 01 -> Memory mode, 16-bit displacement follows
    // 01 -> Register mode, no displacement
    return byte >> 6;
}

uint8_t get_register_field(uint8_t byte) {
    //        W=0 w=1
    // 000 -> AL  AX
    // 001 -> CL  CX
    // 010 -> DL  DX
    // 011 -> BL  BX
    // 100 -> AH  SP
    // 101 -> CH  BP
    // 110 -> DH  SI
    // 111 -> BH  DI
    return (byte & 0xC7) >> 3;
}

uint8_t get_reg_mem_field(uint8_t byte) {
    // See page 4-20, table 4-10 for more info
    return byte & 0x07;
}

uint8_t get_direction(uint8_t byte) {
    // 1 -> destination specified in REG field
    // 0 -> source specified in REG field
    if(byte & 0x02)
        return 1;
    return 0;
}

uint8_t get_width(uint8_t byte) {
    // 1 -> word, 0 -> byte
    return byte & 0x01;
}

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

uint8_t get_h(uint16_t reg_value) {
    reg_value >>= 8;
    return reg_value;
}

uint8_t get_l(uint16_t reg_value) {
    return reg_value & 0xFF;
}

uint8_t jmp_instr(uint8_t opcode, uint8_t *data) {
    // Conditional Jump instructions table:
    // Mnemonic Condition tested        Jump if ... (p69, 2-46)
    // JA/JNBE  (CF OR ZF)=O            above/not below nor equal
    // JAE/JNB  CF=O                    above or equal/ not below       0x73
    // JB/JNAE  CF=1                    below / not above nor equal
    // JBE/JNA  (CF OR ZF)=1            below or equal/ not above
    // JC       CF=1                    carry
    // JE/JZ    ZF=1                    equal/zero
    // JG/JNLE  ((SF XOR OF) OR ZF)=O   greater / not less nor equal
    // JGE/JNL  (SF XOR OF)=O           greater or equal/not less
    // JLlJNGE  (SF XOR OF)=1           less/not greater nor equal
    // JLE/JNG  ((SF XOR OF) OR ZF)=1   less or equal/ not greater
    // JNC      CF=O                    not carry
    // JNE/JNZ  ZF=O                    not equal/ not zero             0x75
    // JNO      OF=O                    not overflow
    // JNP/JPO  PF=O                    not parity / parity odd         0x7B
    // JNS      SF=O                    not sign                        0x79
    // JO       OF=1                    overflow
    // JP/JPE   PF=1                    parity / parity equal
    // JS       SF=1                    sign
    // Relative jump jump relative to the first byte of the next
    // instruction: JMP 0x00 jumps to the first byte of the next instruction
    uint8_t ret_val = 0;
    switch(opcode) {
        case 0x73: {    // JNB/JAE SHORT-LABEL JNC: [0x73, IP-INC8] (p269, 4-30)
            REGS->IP += 2;
            printf("Relative Jump JNB/JAE");
            if (get_flag(CF) == 0) {
                printf(" to 0x%02X\n", data[0]);
                REGS->IP += ((int8_t*)data)[0];
            } else {
                printf(": condition didn't meet: CF != 0\n");
            }
            break;
        }
        case 0x75: {    // JNE/JNZ SHORT-LABEL: [0x75, IP-INC8] (p269, 4-30)
            REGS->IP += 2;
            printf("Relative Jump JNE/JNZ");
            if (get_flag(ZF) == 0) {
                printf(" to 0x%02X\n", data[0]);
                REGS->IP += ((int8_t*)data)[0];
            } else {
                printf(": condition didn't meet: ZF != 0\n");
            }
            break;
        }
        case 0x79: {    // JNS SHORT-LABEL: [0x79, IP-INC8] (p269, 4-30)
            REGS->IP += 2;
            printf("Relative Jump JNS");
            if (get_flag(SF) == 0) {
                printf(" to 0x%02X\n", data[0]);
                REGS->IP += ((int8_t*)data)[0];
            } else {
                printf(": condition didn't meet: SF != 0\n");
            }
            break;
        }
        case 0x7B: {    // JNP/JPO SHORT-LABEL: [0x7B, IP-INC8] (p269, 4-30)
            REGS->IP += 2;
            printf("Relative Jump JNP/JPO");
            if (get_flag(PF) == 0) {
                printf(" to 0x%02X\n", data[0]);
                REGS->IP += ((int8_t*)data)[0];
            } else {
                printf(": condition didn't meet: PF != 0\n");
            }
            break;
        }
        case 0xEA: {    // JMP far label: [0xEA, IP-LO, IP-HI, CS-LO, CS-HI]
            REGS->IP = ((uint16_t*)data)[0];
            REGS->CS = ((uint16_t*)data)[1];
            printf("Far jump to IP = 0x%04X, CS = 0x%04X\n", REGS->IP, REGS->CS);
            break;
        }
        default:
            printf("Error: Unknown JMP instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t mov_instr(uint8_t opcode, uint8_t *data) {
    uint8_t ret_val = 0;
    switch(opcode) {
        case 0xB4: {
            printf("Instruction 0xB4: MOV AH immed = 0x%02X\n", data[0]);
            REGS->AX = set_h(REGS->AX, data[0]);
            ret_val = 2;
            break;
        }
        default:
            printf("Error: Unknown MOV instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t calc_of_flag(uint16_t val1, uint16_t val2, uint16_t res, uint16_t bit) {
    if(((val1 & bit) > 0) && ((val2 & bit) > 0) && (((res) & bit) == 0) ||
        ((val1 & bit) == 0) && ((val2 & bit) == 0) && (((res) & bit) > 0))
        return 1;
    return 0;
}

uint8_t get_parity(uint8_t byte) {
    // Returns 1 if the byte contains an even number of 1-bits, otherwise returns 0
    uint8_t counter = 0;
    for(int i=1; i<0x100; i<<=1) {
        if ((byte & i) > 0)
            counter++;
    }
    return (counter & 0x01)? 0:1;
}

uint8_t cmp_instr(uint8_t opcode, uint8_t *data) {
    // CMP updates AF, CF, OF, PF, SF and ZF
    uint8_t ret_val = 0;
    switch(opcode) {
        case 0x3D: {
            uint32_t val = (data[1] << 8) + data[0];
            uint32_t AX = REGS->AX;
            uint32_t res = val + AX;
            printf("Instruction 0x3D: CMP AX immed16 = 0x%04X\n", val);
            set_flag(AF, ((val & 0x0F) > (AX & 0x0F)));
            set_flag(CF, res > 0xFFFF);
            set_flag(OF, calc_of_flag(val, AX, res, 0x8000));
            set_flag(PF, get_parity(res & 0xFF));
            set_flag(SF, res & 0x8000 > 0);
            set_flag(ZF, res == 0);
            REGS->AX = set_h(REGS->AX, data[0]);
            ret_val = 3;
            break;
        }
        default:
            printf("Error: Unknown CMP instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t add_instr(uint8_t opcode, uint8_t *data) {
    // ADD updates AF, CF, OF, PF, SF and ZF
    uint8_t ret_val = 0;
    switch(opcode) {
        case 0x05: {
            uint32_t val = (data[1] << 8) + data[0];
            uint32_t AX = REGS->AX;
            uint32_t res = val + AX;
            printf("Instruction 0x05: ADD AX = 0x%04X immed = 0x%04X, res = 0x%04X\n", AX, val, res);
            set_flag(AF, ((val & 0x0F) + (REGS->AX & 0x0F) > 0x0F));
            set_flag(CF, res > 0xFFFF);
            set_flag(OF, calc_of_flag(val, AX, res, 0x8000));
            set_flag(PF, get_parity(res & 0xFF));
            set_flag(SF, res & 0x8000 > 0);
            set_flag(ZF, res == 0);
            REGS->AX = res;
            ret_val = 3;
            break;
        }
        default:
            printf("Error: Unknown ADD instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t sahf_instr(void) {
    // Loads the SF, ZF, AF, PF, and CF flags of the EFLAGS register with
    // values from the corresponding bits in the AH register (7, 6, 4, 2, 0 respectively)
    uint8_t AH = get_h(REGS->AX);
    printf("Set flags from AH register: AH = 0x%02X, flags before: 0x%04X, ", AH, REGS->flags);
    set_flag(CF, AH & 0x01);
    set_flag(PF, AH & 0x04);
    set_flag(AF, AH & 0x10);
    set_flag(ZF, AH & 0x40);
    set_flag(SF, AH & 0x80);
    printf("flags after: 0x%04X\n", REGS->flags);
    return 1;
}

uint8_t lahf_instr(void) {
    // Loads lower byte from the flags register into AH register
    printf("Copy flags & 0xFF into AH register: flags = 0x%04X, AX before: 0x%04X, ", REGS->flags, REGS->AX);
    REGS->AX = set_h(REGS->AX, 0xFF & REGS->flags);
    printf("AX after: 0x%04X\n", REGS->AX);
    return 1;
}

uint8_t process_instruction(uint8_t * memory) {
    printf("Processing bytes: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X ", memory[0], memory[1], memory[2], memory[3], memory[4], memory[5]);
    uint8_t ret_val = 1;
    switch(memory[0]) {
        // case 0x00:  // ADD REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x01:  // ADD REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x02:  // ADD REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x03:  // ADD REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x04:  // ADD AL, IMMED8           [DATA-8]
        case 0x05:  // ADD AX, immed16          [0x05, DATA-LO, DATA-HI]
            ret_val = add_instr(memory[0], &memory[1]);
            break;
        // case 0x06:  // PUSH ES
            // push_reg_op(memory);
            // REGS->IP += 1;
        //     break;
        // case 0x07:  // POP ES
            // pop_reg_op(memory[0]);
            // REGS->IP += 1;
            // break;
        // case 0x08:  // OR REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x09:  // OR REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x0A:  // OR REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x0B:  // OR REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x0C:  // OR AL, IMMED8           [DATA-8]
        // case 0x0D:  // OR AX, immed16          [DATA-LO, DATA-HI]
        //     REGS->IP += or_op(memory);
        //     break;
        // case 0x0E:  // PUSH CS
        //     push_reg_op(memory[0]);
        //     REGS->IP += 1;
        //     break;
        // case 0x0F:  // NOT USED
        //     INVALID_INSTRUCTION;
        //     break;
        // case 0x10:  // ADC REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x11:  // ADC REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x12:  // ADC REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x13:  // ADC REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x14:  // ADC AL, IMMED8           [DATA-8]
        // case 0x15:  // ADC AX, immed16          [DATA-LO, DATA-HI]
        //     REGS->IP += adc_op(memory);
        //     break;
        // case 0x16:  // PUSH SS
        //     push_reg_op(memory[0]);
        //     REGS->IP += 1;
        //     break;
        // case 0x17:  // POP SS
        //     pop_reg_op(memory[0]);
        //     REGS->IP += 1;
        //     break;
        // case 0x18:  // SBB REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x19:  // SBB REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x1A:  // SBB REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x1B:  // SBB REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x1C:  // SBB AL, IMMED8           [DATA-8]
        // case 0x1D:  // SBB AX, immed16          [DATA-LO, DATA-HI]
        //     REGS->IP += sbb_op(memory);
        //     break;
        // case 0x1E:  // PUSH DS
        //     push_reg_op(memory[0]);
        //     REGS->IP += 1;
        //     break;
        // case 0x1F:  // POP DS
        //     pop_reg_op(memory[0]);
        //     REGS->IP += 1;
        //     break;
        // case 0x20:  // AND REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x21:  // AND REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x22:  // AND REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x23:  // AND REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x24:  // AND AL, IMMED8           [DATA-8]
        // case 0x25:  // AND AX, immed16          [DATA-LO, DATA-HI]
        //     REGS->IP += and_op(memory);
        //     break;
        // case 0x26:  // ES:
        //     segment_override(ES);  // ES: segment overrige prefix
        //     REGS->IP += 1;
        //     break;
        // case 0x27:  // DAA
        //     daa_op();
        //     REGS->IP += 1;
        //     break;
        // case 0x28:  // SUB REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x29:  // SUB REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x2A:  // SUB REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x2B:  // SUB REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x2C:  // SUB AL, IMMED8           [DATA-8]
        // case 0x2D:  // SUB AX, immed16          [DATA-LO, DATA-HI]
        //     REGS->IP += sub_op(memory);
        //     break;
        // case 0x2E:  // CS:
        //     segment_override(CS);  // CS: segment overrige prefix
        //     REGS->IP += 1;
        //     break;
        // case 0x2F:  // DAS
        //     das_op();
        //     REGS->IP += 1;
        //     break;
        // case 0x30:  // XOR REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x31:  // XOR REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x32:  // XOR REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x33:  // XOR REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x34:  // XOR AL, IMMED8           [DATA-8]
        // case 0x35:  // XOR AX, immed16          [DATA-LO, DATA-HI]
        //     REGS->IP += xor_op(memory);
        //     break;
        // case 0x36:  // SS:
        //     segment_override(SS);  // SS: segment overrige prefix
        //     REGS->IP += 1;
        //     break;
        // case 0x37:  // AAA
        //     aaa_op();
        //     REGS->IP += 1;
        //     break;
        // case 0x38:  // CMP REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x39:  // CMP REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x3A:  // CMP REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x3B:  // CMP REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        // case 0x3C:  // CMP AL, IMMED8           [DATA-8]
        case 0x3D:  // CMP AX, immed16: [0x3D, DATA-LO, DATA-HI]
            ret_val = cmp_instr(memory[0], &memory[1]);
            break;
        // case 0x3E:  // DS:
        //     segment_override(DS);  // DS: segment overrige prefix
        //     REGS->IP += 1;
        //     break;
        // case 0x3F:  // AAS
        //     aas_op();
        //     REGS->IP += 1;
        //     break;
        // case 0x40:  // INC AX
        // case 0x41:  // INC CX
        // case 0x42:  // INC DX
        // case 0x43:  // INC BX
        // case 0x44:  // INC SP
        // case 0x45:  // INC BP
        // case 0x46:  // INC SI
        // case 0x47:  // INC DI
        //     inc_register(memory);
        //     REGS->IP += 1;
        //     break;
        // case 0x48:  // DEC AX
        // case 0x49:  // DEC CX
        // case 0x4A:  // DEC DX
        // case 0x4B:  // DEC BX
        // case 0x4C:  // DEC SP
        // case 0x4D:  // DEC BP
        // case 0x4E:  // DEC SI
        // case 0x4F:  // DEC DI
        //     dec_register(memory);
        //     REGS->IP += 1;
        //     break;
        // case 0x50:  // PUSH AX
        // case 0x51:  // PUSH CX
        // case 0x52:  // PUSH DX
        // case 0x53:  // PUSH BX
        // case 0x54:  // PUSH SP
        // case 0x55:  // PUSH BP
        // case 0x56:  // PUSH SI
        // case 0x57:  // PUSH DI
        //     push_reg_op(memory);
        //     REGS->IP += 1;
        //     break;
        // case 0x58:  // POP AX
        // case 0x59:  // POP CX
        // case 0x5A:  // POP DX
        // case 0x5B:  // POP BX
        // case 0x5C:  // POP SP
        // case 0x5D:  // POP BP
        // case 0x5E:  // POP SI
        // case 0x5F:  // POP DI
        //     pop_reg_op(memory[0]);
        //     REGS->IP += 1;
        //     break;
        // case 0x60:
        // case 0x61:
        // case 0x62:
        // case 0x63:
        // case 0x64:
        // case 0x65:
        // case 0x66:
        // case 0x67:
        // case 0x68:
        // case 0x69:
        // case 0x6A:
        // case 0x6B:
        // case 0x6C:
        // case 0x6D:
        // case 0x6E:
        // case 0x6F:
        //     INVALID_INSTRUCTION;
        //     break;
        // case 0x70:
        //     if(jo_op())
        //         REGS->IP = memory[1];
        //     break;
        // case 0x71:
        //     if(!jo_op())
        //         REGS->IP = memory[1];
        //     break;
        // case 0x72:
        //     if(jb_op())
        //         REGS->IP = memory[1];
        //     break;
        case 0x73:  // JNB/JAE SHORT-LABEL JNB: [0x73, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        // case 0x74:
        //     if(je_op())
        //         REGS->IP = memory[1];
        //     break;
        case 0x75:  // JNE/JNZ SHORT LABEL: [0x75, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        // case 0x76:
        //     if(jbe_op())
        //         REGS->IP = memory[1];
        //     break;
        // case 0x77:
        //     if(!jbe_op())
        //         REGS->IP = memory[1];
        //     break;
        // case 0x78:
        //     if(js_op())
        //         REGS->IP = memory[1];
        //     break;
        case 0x79:  // JNS SHORT LABEL: [0x79, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        // case 0x7A:
        //     if(jp_op())
        //         REGS->IP = memory[1];
        //     break;
        case 0x7B:  // JNP/JPO SHORT LABEL: [0x7B, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        // case 0x7C:
        //     if(jl_op())
        //         REGS->IP = memory[1];
        //     break;
        // case 0x7D:
        //     if(!jl_op())
        //         REGS->IP = memory[1];
        //     break;
        // case 0x7E:
        //     if(jle_op())
        //         REGS->IP = memory[1];
        //     break;
        // case 0x7F:
        //     if(!jle_op())
        //         REGS->IP = memory[1];
        //     break;
        // case 0x80:
        //     // 8-bit operations
        //     switch(get_mode_field(memory[1])) {
        //         case 0:
        //             REGS->IP += add_op(memory);
        //             break;
        //         case 1:
        //             REGS->IP += or_op(memory);
        //             break;
        //         case 2:
        //             REGS->IP += adc_op(memory);
        //             break;
        //         case 3:
        //             REGS->IP += sbb_op(memory);
        //             break;
        //         case 4:
        //             REGS->IP += and_op(memory[1], &memory[2]);
        //             break;
        //         case 5:
        //             REGS->IP += sub_op(memory[1], &memory[2]);
        //             break;
        //         case 6:
        //             REGS->IP += xor_op(memory);
        //             break;
        //         case 7:
        //             REGS->IP += cmp_op(memory);
        //             break;
        //     }
        //     break;
        // case 0x81:
        //     // 16-bit operations
        //     switch(get_mode_field(memory[1])) {
        //         case 0:
        //             REGS->IP += add_op(memory);
        //             break;
        //         case 1:
        //             REGS->IP += or_op(memory);
        //             break;
        //         case 2:
        //             REGS->IP += adc_op(memory);
        //             break;
        //         case 3:
        //             REGS->IP += sbb_op(memory);
        //             break;
        //         case 4:
        //             REGS->IP += and_op(memory[1], &memory[2]);
        //             break;
        //         case 5:
        //             REGS->IP += sub_op(memory[1], &memory[2]);
        //             break;
        //         case 6:
        //             REGS->IP += xor_op(memory);
        //             break;
        //         case 7:
        //             REGS->IP += cmp_op(memory);
        //             break;
        //     }
        //     break;
        // case 0x82:
        //     // 8-bit operations
        //     switch(get_mode_field(memory[1])) {
        //         case 0:
        //             REGS->IP += add_op(memory);
        //             break;
        //         case 1:
        //             INVALID_INSTRUCTION;
        //             break;
        //         case 2:
        //             REGS->IP += adc_op(memory);
        //             break;
        //         case 3:
        //             REGS->IP += sbb_op(memory);
        //             break;
        //         case 4:
        //             INVALID_INSTRUCTION;
        //             break;
        //         case 5:
        //             REGS->IP += sub_op(memory[1], &memory[2]);
        //             break;
        //         case 6:
        //             INVALID_INSTRUCTION;
        //             break;
        //         case 7:
        //             REGS->IP += cmp_op(memory);
        //             break;
        //     }
        //     break;
        // case 0x83:
        //     // 16-bit operations
        //     switch(get_mode_field(memory[1])) {
        //         case 0:
        //             REGS->IP += add_op(memory);
        //             break;
        //         case 1:
        //             INVALID_INSTRUCTION;
        //             break;
        //         case 2:
        //             REGS->IP += adc_op(memory);
        //             break;
        //         case 3:
        //             REGS->IP += sbb_op(memory);
        //             break;
        //         case 4:
        //             INVALID_INSTRUCTION;
        //             break;
        //         case 5:
        //             REGS->IP += sub_op(memory[1], &memory[2]);
        //             break;
        //         case 6:
        //             INVALID_INSTRUCTION;
        //             break;
        //         case 7:
        //             REGS->IP += cmp_op(memory);
        //             break;
        //     }
        //     break;
        // case 0x84:
        //     REGS->IP += test_8b_op(memory[1], &memory[2]);
        //     break;
        // case 0x85:
        //     REGS->IP += test_16b_op(memory[1], &memory[2]);
        //     break;
        // case 0x86:
        //     REGS->IP += xchg_8b_op(memory[1], &memory[2]);
        //     break;
        // case 0x87:
        //     REGS->IP += xchg_16b_op(memory[1], &memory[2]);
        //     break;
        // case 0x88:
        //     REGS->IP += mov_16b_op(memory[1], &memory[2]);
        //     break;
        // case 0x89:
        //     REGS->IP += mov_16b_op(memory[1], &memory[2]);
        //     break;
        // case 0x8A:
        //     REGS->IP += mov_16b_op(memory[1], &memory[2]);
        //     break;
        // case 0x8B:
        //     REGS->IP += mov_16b_op(memory[1], &memory[2]);
        //     break;
        // case 0x8C:
        //     if(3 & get_mode_field(memory[1]))
        //         INVALID_INSTRUCTION;
        //     else
        //         REGS->IP += mov_16b_op(memory[1], &memory[2]);
        //     break;
        // case 0x8D:  // LEA REG16, MEM16
        //     REGS->IP += lea_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
        //     break;
        // case 0x8E:
        //     if((3 & get_mode_field(memory[1])) == 0)    // MOV SEGREG, REG16/MEM16
        //         REGS->IP += mov_16b_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
        //     else
        //         INVALID_INSTRUCTION;
        //     break;
        // case 0x8F:
        //     if(0 == get_mode_field(memory[1]))  // POP REG16/MEM16
        //         REGS->IP += pop_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
        //     else
        //         INVALID_INSTRUCTION;
        //     break;
        // case 0x90:  // NOP, XCHG AX AX
        //         REGS->IP += nop_op();
        //     break;
        // case 0x91:  // XCHG AX CX
        //         REGS->IP += xchg_reg_op(AX, CX);
        //     break;
        // case 0x92:  // XCHG AX, DX
        //         REGS->IP += xchg_reg_op(AX, DX);
        //     break;
        // case 0x93:  // XCHG AX, BX
        //         REGS->IP += xchg_reg_op(AX, BX);
        //     break;
        // case 0x94:  // XCHG AX, SP
        //         REGS->IP += xchg_reg_op(AX, SP);
        //     break;
        // case 0x95:  // XCHG AX, BP
        //         REGS->IP += xchg_reg_op(AX, BP);
        //     break;
        // case 0x96:  // XCHG AX, SI
        //         REGS->IP += xchg_reg_op(AX, SI);
        //     break;
        // case 0x97:  // XCHG AX, DI
        //         REGS->IP += xchg_reg_op(AX, DI);
        //     break;
        // case 0x98:  // CBW
        //         REGS->IP += cbw_op();
        //     break;
        // case 0x99:  // CWD
        //         REGS->IP += cwd_op();
        //     break;
        // case 0x9A:  // CALL FAR_PROC
        //     REGS->IP += call_op(memory[1], &memory[2]); // [DISP-LO, DISP-HIGH, SEG-LO, SEG-HI]
        //     break;
        // case 0x9B:  // WAIT
        //     REGS->IP += wait_op();
        //     break;
        // case 0x9C:  // PUSHF
        //     REGS->IP += pushf_op();
        //     break;
        // case 0x9D:  // POPF
        //     REGS->IP += popf_op();
        //     break;
        case 0x9E:  // SAHF
            ret_val = sahf_instr();
            break;
        case 0x9F:  // LAHF
            REGS->IP += lahf_instr();
            break;
        // case 0xA0:  // MOV AL, MEM8
        //     REGS->IP += mov_op(memory[1], &memory[2]); // ADDR-LO, ADDR-HI
        //     break;
        // case 0xA1:  // MOV AX, MEM16
        //     REGS->IP += mov_op(memory[1], &memory[2]); // ADDR-LO, ADDR-HI
        //     break;
        // case 0xA2:  // MOV MEM8, AL
        //     REGS->IP += mov_op(memory[1], &memory[2]); // ADDR-LO, ADDR-HI
        //     break;
        // case 0xA3:  // MOV MEM8, AX
        //     REGS->IP += mov_op(memory[1], &memory[2]); // ADDR-LO, ADDR-HI
        //     break;
        // case 0xA4:  // MOVS DEST-STR8, SRC-STR8
        //     REGS->IP += mov_str_op(memory[1], &memory[2]);
        //     break;
        // case 0xA5:  // MOVS DEST-STR16, SRC-STR16
        //     REGS->IP += mov_str_op(memory[1], &memory[2]);
        //     break;
        // case 0xA6:  // CMPSS DEST-STR8, SRC-STR8
        //     REGS->IP += cmps_str_op(memory[1], &memory[2]);
        //     break;
        // case 0xA7:  // CMPSS DEST-STR16, SRC-STR16
        //     REGS->IP += cmps_str_op(memory[1], &memory[2]);
        //     break;
        // case 0xA8:  // TEST AL, IMMED8
        //     REGS->IP += cmps_str_op(memory[1], &memory[2]); // DATA8
        //     break;
        // case 0xA9:  // TEST AX, IMMED16
        //     REGS->IP += cmps_str_op(memory[1], &memory[2]); // DATA_LO, DATA-HI
        //     break;
        // case 0xAA:  // STOS DEST-STR8
        //     REGS->IP += stos_str_op(memory[1], &memory[2]);
        //     break;
        // case 0xAB:  // STOS DEST-STR16
        //     REGS->IP += stos_str_op(memory[1], &memory[2]);
        //     break;
        // case 0xAC:  // LODS DEST-STR8
        //     REGS->IP += stos_str_op(memory[1], &memory[2]);
        //     break;
        // case 0xAD:  // LODS DEST-STR16
        //     REGS->IP += stos_str_op(memory[1], &memory[2]);
        //     break;
        // case 0xAE:  // SCAS DEST-STR8
        //     REGS->IP += stos_str_op(memory[1], &memory[2]);
        //     break;
        // case 0xAF:  // SCAS DEST-STR16
        //     REGS->IP += stos_str_op(memory[1], &memory[2]);
        //     break;
        // case 0xB0:  // MOV AL, IMMED8
        //     REGS->AX = set_l(REGS->AX, memory[1]); // DATA-8
        //     REGS->IP += 2;
        //     break;
        // case 0xB1:  // MOV CL, IMMED8
        //     REGS->CX = set_l(REGS->CX, memory[1]); // DATA-8
        //     REGS->IP += 2;
        //     break;
        // case 0xB2:  // MOV DL, IMMED8
        //     REGS->DX = set_l(REGS->DX, memory[1]); // DATA-8
        //     REGS->IP += 2;
        //     break;
        // case 0xB3:  // MOV BL, IMMED8
        //     REGS->BX = set_l(REGS->BX, memory[1]); // DATA-8
        //     REGS->IP += 2;
        //     break;
        case 0xB4:  // MOV AH, IMMED8
            ret_val = mov_instr(memory[0], &memory[1]);
            break;
        // case 0xB5:  // MOV CH, IMMED8
        //     REGS->CX = set_h(REGS->CX, memory[1]); // DATA-8
        //     REGS->IP += 2;
        //     break;
        // case 0xB6:  // MOV DH, IMMED8
        //     REGS->DX = set_h(REGS->DX, memory[1]); // DATA-8
        //     REGS->IP += 2;
        //     break;
        // case 0xB7:  // MOV BH, IMMED8
        //     REGS->BX = set_h(REGS->BX, memory[1]); // DATA-8
        //     REGS->IP += 2;
        //     break;
        // case 0xB8:  // MOV AX, IMMED16
        //     REGS->AX = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
        //     REGS->IP += 3;
        //     break;
        // case 0xB9:  // MOV CX, IMMED16
        //     REGS->CX = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
        //     REGS->IP += 3;
        //     break;
        // case 0xBA:  // MOV DX, IMMED16
        //     REGS->DX = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
        //     REGS->IP += 3;
        //     break;
        // case 0xBB:  // MOV BX, IMMED16
        //     REGS->BX = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
        //     REGS->IP += 3;
        //     break;
        // case 0xBC:  // MOV SP, IMMED16
        //     REGS->SP = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
        //     REGS->IP += 3;
        //     break;
        // case 0xBD:  // MOV BP, IMMED16
        //     REGS->BP = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
        //     REGS->IP += 3;
        //     break;
        // case 0xBE:  // MOV SI, IMMED16
        //     REGS->SI = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
        //     REGS->IP += 3;
        //     break;
        // case 0xBF:  // MOV DI, IMMED16
        //     REGS->DI = (memory[2] << 8) + memory[1]; // DATA-LO, DATA-HI
        //     REGS->IP += 3;
        //     break;
        // case 0xC0:
        //     INVALID_INSTRUCTION;
        //     break;
        // case 0xC1:
        //     INVALID_INSTRUCTION;
        //     break;
        // case 0xC2:  // RET IMMED16 (intrasegment)
        //     REGS->IP += ret_op(memory[1], &memory[2]);  // DATA-LO, DATA-HI
        //     break;
        // case 0xC3:  // RET (intrasegment)
        //     REGS->IP += ret_op(memory[1], &memory[2]);
        //     break;
        // case 0xC4:  // LES REG16, MEM16
        //     REGS->IP += les_op(memory[1], &memory[2]);  // MOD REG R/M, DISP-LO, DISP-HI
        //     break;
        // case 0xC5:  // LDS REG16, MEM16
        //     REGS->IP += lds_op(memory[1], &memory[2]);  // MOD REG R/M, DISP-LO, DISP-HI
        //     break;
        // case 0xC6:  // MOV MEM8, IMMED8
        //     if(get_mode_field(memory[1]) == 0) {
        //         REGS->IP += mov_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI, DATA-8
        //     } else {
        //         INVALID_INSTRUCTION;
        //     }
        //     break;
        // case 0xC7:  // MOV MEM16, IMMED16
        //     if(get_mode_field(memory[1]) == 0) {
        //         REGS->IP += mov_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI, DATA-LO, DATA-HI
        //     } else {
        //         INVALID_INSTRUCTION;
        //     }
        //     break;
        // case 0xC8:
        //     INVALID_INSTRUCTION;
        //     break;
        // case 0xC9:
        //     INVALID_INSTRUCTION;
        //     break;
        // case 0xCA:  // RET IMMED16 (intersegment)
        //     REGS->IP += ret_op(memory[1], &memory[2]);  // DATA-LO, DATA-HI
        //     break;
        // case 0xCB:  // RET (intersegment)
        //     REGS->IP += ret_op(memory[1], &memory[2]);
        //     break;
        // case 0xCC:  // INT 3
        //     REGS->IP += int_op(3);
        //     break;
        // case 0xCD:  // INT IMMED8
        //     REGS->IP += int_op(memory[1]);  // DATA-8
        //     break;
        // case 0xCE:  // INTO
        //     REGS->IP += into_op();
        //     break;
        // case 0xCF:  // IRET
        //     REGS->IP += iret_op();
        //     break;
        // case 0xD0:
        //     // 8-bit operations
        //     switch(get_mode_field(memory[1])) {
        //         case 0: // ROL REG8/MEM8, 1
        //             REGS->IP += rol_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 1: // ROR REG8/MEM8, 1
        //             REGS->IP += ror_op(memory[1], &memory[2]);  // MOD 001 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 2: // RCL REG8/MEM8, 1
        //             REGS->IP += rcl_op(memory[1], &memory[2]);  // MOD 010 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 3: // RCR REG8/MEM8, 1
        //             REGS->IP += rcr_op(memory[1], &memory[2]);  // MOD 011 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 4: // SAL/SHL REG8/MEM8, 1
        //             REGS->IP += shl_op(memory[1], &memory[2]);  // MOD 100 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 5: // SHR REG8/MEM8, 1
        //             REGS->IP += shr_op(memory[1], &memory[2]);  // MOD 101 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 6:
        //             INVALID_INSTRUCTION;    // MOD 110 R/M,
        //             break;
        //         case 7: // SAR REG8/MEM8, 1
        //             REGS->IP += sar_op(memory[1], &memory[2]);  // MOD 111 R/M, DISP-LO, DISP-HI
        //             break;
        //     }
        //     break;
        // case 0xD1:
        //     // 16-bit operations
        //     switch(get_mode_field(memory[1])) {
        //         case 0: // ROL REG16/MEM16, 1
        //             REGS->IP += rol_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 1: // ROR REG16/MEM16, 1
        //             REGS->IP += ror_op(memory[1], &memory[2]);  // MOD 001 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 2: // RCL REG16/MEM16, 1
        //             REGS->IP += rcl_op(memory[1], &memory[2]);  // MOD 010 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 3: // RCR REG16/MEM16, 1
        //             REGS->IP += rcr_op(memory[1], &memory[2]);  // MOD 01 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 4: // SAL/SHL REG16/MEM16, 1
        //             REGS->IP += shl_op(memory[1], &memory[2]);  // MOD 100 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 5: // SHR REG16/MEM16, 1
        //             REGS->IP += shr_op(memory[1], &memory[2]);  // MOD 101 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 6:
        //             INVALID_INSTRUCTION;    // MOD 110 R/M,
        //             break;
        //         case 7: // SAR REG16/MEM16, 1
        //             REGS->IP += sar_op(memory[1], &memory[2]);  // MOD 111 R/M, DISP-LO, DISP-HI
        //             break;
        //     }
        //     break;
        // case 0xD2:
        //     // 8-bit operations
        //     switch(get_mode_field(memory[1])) {
        //         case 0: // ROL REG8/MEM8, CL
        //             REGS->IP += rol_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 1: // ROR REG8/MEM8, CL
        //             REGS->IP += ror_op(memory[1], &memory[2]);  // MOD 001 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 2: // RCL REG8/MEM8, CL
        //             REGS->IP += rcl_op(memory[1], &memory[2]);  // MOD 010 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 3: // RCR REG8/MEM8, CL
        //             REGS->IP += rcr_op(memory[1], &memory[2]);  // MOD 011 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 4: // SAL/SHL REG8/MEM8, CL
        //             REGS->IP += shl_op(memory[1], &memory[2]);  // MOD 100 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 5: // SHR REG8/MEM8, CL
        //             REGS->IP += shr_op(memory[1], &memory[2]);  // MOD 101 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 6:
        //             INVALID_INSTRUCTION;    // MOD 110 R/M,
        //             break;
        //         case 7: // SAR REG8/MEM8, CL
        //             REGS->IP += sar_op(memory[1], &memory[2]);  // MOD 111 R/M, DISP-LO, DISP-HI
        //             break;
        //     }
        //     break;
        // case 0xD3:
        //     // 16-bit operations
        //     switch(get_mode_field(memory[1])) {
        //         case 0: // ROL REG16/MEM16, CL
        //             REGS->IP += rol_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 1: // ROR REG16/MEM16, CL
        //             REGS->IP += ror_op(memory[1], &memory[2]);  // MOD 001 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 2: // RCL REG16/MEM16, CL
        //             REGS->IP += rcl_op(memory[1], &memory[2]);  // MOD 010 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 3: // RCR REG16/MEM16, CL
        //             REGS->IP += rcr_op(memory[1], &memory[2]);  // MOD 011 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 4: // SAL/SHL REG16/MEM16, CL
        //             REGS->IP += shl_op(memory[1], &memory[2]);  // MOD 100 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 5: // SHR REG16/MEM16, CL
        //             REGS->IP += shr_op(memory[1], &memory[2]);  // MOD 101 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 6:
        //             INVALID_INSTRUCTION;    // MOD 110 R/M,
        //             break;
        //         case 7: // SAR REG16/MEM16, CL
        //             REGS->IP += sar_op(memory[1], &memory[2]);  // MOD 111 R/M, DISP-LO, DISP-HI
        //             break;
        //     }
        //     break;
        // case 0xD4:
        //     if(memory[1] == 0b00001010) {  // AAM
        //         REGS->IP += aam_op(memory[1]);  // memory[1] == 0b00001010
        //     } else {
        //         INVALID_INSTRUCTION;
        //     }
        //     break;
        // case 0xD5:
        //     if(memory[1] == 0b00001010) {  // AAD
        //         REGS->IP += aam_op(memory[1]);  // memory[1] == 0b00001010
        //     } else {
        //         INVALID_INSTRUCTION;
        //     }
        //     break;
        // case 0xD6:
        //     INVALID_INSTRUCTION;
        //     break;
        // case 0xD7:  // XLAT SOURCE-TABLE
        //     REGS->IP += xlat_op();
        //     break;
        // case 0xD8:  // ESC OPCODE, SOURCE
        // case 0xD9:
        // case 0xDA:
        // case 0xDB:
        // case 0xDC:
        // case 0xDD:
        // case 0xDE:
        // case 0xDF:
        //     if((memory[0] == 0xD8 && get_register_field(memory[1]) == 0) ||
        //        (memory[0] == 0xDF && get_register_field(memory[1]) == 0x07)) {
        //         REGS->IP += esc_op();   // MOD 000/111 R/M; TODO: Not sure
        //     } else {
        //         REGS->IP += esc_op(memory[1], &memory[2]);  // MOD 000/111 R/M, DISP-LO, DISP-HI
        //     }
        //     break;
        // case 0xE0:  // LOOPNE/LOOPNZ SHORT-LABEL
        //     // LOOPNE decrements ecx and checks that ecx is not zero and ZF is not set - if these
        //     // conditions are met, it jumps at label, otherwise falls through
        //     REGS->CX--;
        //     if(REGS->CX > 0 && REGS->flags.ZF == 0) {
        //         REGS->IP += memory[1];  // IP-INC-8
        //     } else {
        //         REGS->IP += 2;
        //     }
        //     break;
        // case 0xE1:  // LOOPE/LOOPZ SHORT-LABEL
        //     // LOOPE decrements ecx and checks that ecx is not zero and ZF is set - if these
        //     // conditions are met, it jumps at label, otherwise falls through
        //     REGS->CX--;
        //     if(REGS->CX > 0 && REGS->flags.ZF) {
        //         REGS->IP += memory[1];  // IP-INC-8
        //     } else {
        //         REGS->IP += 2;
        //     }
        //     break;
        // case 0xE2:  // LOOP SHORT-LABEL
        //     // LOOP decrements ecx and checks if ecx is not zero, if that 
        //     // condition is met it jumps at specified label, otherwise falls through
        //     REGS->CX--;
        //     if(REGS->CX > 0) {
        //         REGS->IP += memory[1];  // IP-INC-8
        //     } else {
        //         REGS->IP += 2;
        //     }
        //     break;
        // case 0xE3:  // JPXZ
        //     // JCXZ (Jump if ECX Zero) branches to the label specified in the instruction if it finds a value of zero in ECX
        //     if(REGS->CX == 0) {
        //         REGS->IP += memory[1];  // IP-INC-8
        //     } else {
        //         REGS->IP += 2;
        //     }
        //     break;
        // case 0xE4:  // IN AL, IMMED8
        //     REGS->AX = set_l(REGS->AX, get_io_8bit(memory[1]));  // DATA-8
        //     REGS->IP += 2;
        //     break;
        // case 0xE5:  // IN AX, IMMED8
        //     REGS->AX = get_io_16bit(memory[1]);  // DATA-8
        //     REGS->IP += 2;
        //     break;
        // case 0xE6:  // OUT AL, IMMED8
        //     set_io_8bit(memory[1], get_l(REGS->AX)); // DATA-8
        //     REGS->IP += 2;
        //     break;
        // case 0xE7:  // OUT AX, IMMED8
        //     set_io_16bit(memory[1], REGS->AX); // DATA-8
        //     REGS->IP += 2;
        //     break;
        // case 0xE8:  // CALL NEAR-PROC
        //     uint16_t addr = memory[2];
        //     addr <<= 8;
        //     addr += memory[1];
        //     REGS->IP += addr;   // IP-INC-LO, IP-INC-HI
        //     break;
        // case 0xE9:  // JMP NEAR-LABEL
        //     uint16_t addr = memory[2];
        //     addr <<= 8;
        //     addr += memory[1];
        //     REGS->IP += addr;   // IP-INC-LO, IP-INC-HI
        //     break;
        case 0xEA:  // JMP FAR-LABEL: [0xEA, IP-LO, IP-HI, CS-LO, CS-HI]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        // case 0xEB:  // JMP SHORT-LABEL
        //     REGS->IP += memory[1];
        //     break;
        // case 0xEC:  // IN AL, DX
        //     REGS->AX = set_l(REGS->AX, get_io_8bit(REGS->DX));
        //     REGS->IP += 1;
        //     break;
        // case 0xED:  // IN AX, DX
        //     REGS->AX = get_io_16bit(REGS->DX);  // DATA-8
        //     REGS->IP += 1;
        //     break;
        // case 0xEE:  // OUT AL, DX
        //     set_io_8bit(REGS->DX, get_l(REGS->AX)); // DATA-8
        //     REGS->IP += 2;
        //     break;
        // case 0xEF:  // OUT AX, DX
        //     set_io_16bit(REGS->DX, REGS->AX); // DATA-8
        //     REGS->IP += 1;
        //     break;
        // case 0xF0:  // LOCK (prefix)
        //     assert_LOCK_pin();
        //     REGS->IP += 1;
        //     break;
        // case 0xF1:
        //     INVALID_INSTRUCTION;
        //     break;
        // case 0xF2:  // REPNE/REPNZ
        //     while(REGS->CX != 0) {
        //         execute_next_command();
        //         REGS->CX --;
        //     }
        //     break;
        // case 0xF3:  // REP/REPE/REPZ
        //     while(REGS->CX != 0) {
        //         execute_next_command();
        //         REGS->CX --;
        //     }
        //     break;
        // case 0xF4:  // HLT
        //     // halts the CPU until the next external interrupt is fired
        //     break;
        // case 0xF5:  // CMC
        //     // Inverts the CF flag
        //     if(REGS->flags.CF) {
        //         REGS->flags.CF = 0;
        //     } else {
        //         REGS->flags.CF = 1;
        //     }
        //     REGS->IP += 1;
        //     break;
        // case 0xF6:
        //     switch(get_mode_field(memory[1])) {
        //         case 0: // TEST REG8/MEM8, IMMED8
        //             // TEST sets the zero flag, ZF, when the result of the AND operation is zero.
        //             // If two operands are equal, their bitwise AND is zero when both are zero.
        //             // TEST also sets the sign flag, SF, when the most significant bit is set in
        //             // the result, and the parity flag, PF, when the number of set bits is even.
        //             REGS->IP += test_8b_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI, DATA-8
        //             break;
        //         case 1:
        //             INVALID_INSTRUCTION;
        //             break;
        //         case 2: // NOT REG8/MEM8
        //             REGS->IP += not_op(memory[1], &memory[2]);  // NOT 010 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 3: // NEG REG8/MEM8
        //             // subtracts the operand from zero and stores the result in the same operand.
        //             // The NEG instruction affects the carry, overflow, sign, zero, and parity flags according to the result.
        //             REGS->IP += neg_op(memory[1], &memory[2]);  // NEG 010 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 4: // MUL REG8/MEM8
        //             // the source operand (in a general-purpose register or memory location) is multiplied by the value in the AL, AX, EAX, or RAX
        //             // register (depending on the operand size) and the product (twice the size of the input operand) is stored in the
        //             // AX, DX:AX, EDX:EAX, or RDX:RAX registers, respectively
        //             REGS->IP += mul_op(memory[1], &memory[2]);  // MUL 010 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 5: // IMUL REG8/MEM8 (signed)
        //             REGS->IP += mul_op(memory[1], &memory[2]);  // IMUL 010 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 6: // DIV REG8/MEM8
        //             REGS->IP += div_op(memory[1], &memory[2]);  // DIV 010 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 7: // IDIV REG8/MEM8
        //             REGS->IP += idiv_op(memory[1], &memory[2]);  // IDIV 010 R/M, DISP-LO, DISP-HI
        //             break;
        //     }
        //     break;
        // case 0xF7:
        //     switch(get_mode_field(memory[1])) {
        //         case 0: // TEST REG16/MEM16, IMMED16
        //             REGS->IP += test_16b_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI, DATA-16
        //             break;
        //         case 1:
        //             INVALID_INSTRUCTION;
        //             break;
        //         case 2: // NOT REG16/MEM16
        //             REGS->IP += not_op(memory[1], &memory[2]);  // NOT 010 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 3: // NEG REG16/MEM16
        //             REGS->IP += neg_op(memory[1], &memory[2]);  // NEG 010 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 4: // MUL REG16/MEM16
        //             REGS->IP += mul_op(memory[1], &memory[2]);  // MUL 010 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 5: // IMUL REG16/MEM16 (signed)
        //             REGS->IP += mul_op(memory[1], &memory[2]);  // IMUL 010 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 6: // DIV REG16/MEM16
        //             REGS->IP += div_op(memory[1], &memory[2]);  // DIV 010 R/M, DISP-LO, DISP-HI
        //             break;
        //         case 7: // IDIV REG16/MEM16
        //             REGS->IP += idiv_op(memory[1], &memory[2]);  // IDIV 010 R/M, DISP-LO, DISP-HI
        //             break;
        //     }
        //     break;
        // case 0xF8:  // CLC
        //     // Clear Carry Flag
        //     REGS->flags.CF = 0;
        //     REGS->IP += 1;
        //     break;
        // case 0xF9:  // STC
        //     // Set Carry Flag
        //     REGS->flags.CF = 1;
        //     REGS->IP += 1;
        //     break;
        case 0xFA:  // CLI
            // Clear Interrupt Flag, causes the processor to ignore maskable external interrupts
            printf("Instruction 0xFA: Clear Interrupt Flag (IF) to disable interrupts\n");
            set_flag(IF, 0);
            break;
        // case 0xFB:  // STI
        //     // Set Interrupt Flag
        //     REGS->flags.IF = 1;
        //     REGS->IP += 1;
        //     break;
        // case 0xFC:  // CLD
        //     // Clear Direction Flag
        //     // when the direction flag is 0, the instructions work by incrementing the pointer
        //     // to the data after every iteration, while if the flag is 1, the pointer is decremented
        //     REGS->flags.DF = 0;
        //     REGS->IP += 1;
        //     break;
        // case 0xFD:  // CLD
        //     // Set Direction Flag
        //     REGS->flags.DF = 1;
        //     REGS->IP += 1;
        //     break;
        // case 0xFE:
        //     break;
        // case 0xFF:
        //     break;
        default:
            printf("Unknown instruction: 0x%02X\n", memory[0]);
            // INVALID_INSTRUCTION;
    }
    return ret_val;
}

int init_cpu(uint8_t *memory) {
    MEMORY = memory;
    REGS = calloc(1, sizeof(registers_t));
    REGS->IP = 0xFFF0;
    REGS->CS = 0xF000;
    return EXIT_SUCCESS;
}
int cpu_tick(void) {
    if (REGS->ticks < 20) {
        REGS->ticks++;
        REGS->IP += process_instruction(&MEMORY[get_addr(REGS->CS, REGS->IP)]);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
