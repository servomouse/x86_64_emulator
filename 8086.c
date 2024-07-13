// https://en.wikipedia.org/wiki/Intel_8086
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

uint8_t *IO_SPACE = NULL;
uint8_t *MEMORY = NULL;

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
        unsigned int DF : 1;    // Direction flag, used to influence the direction in which string instructions offset pointer registers
        unsigned int IF : 1;    // Interrupt flag
        unsigned int TF : 1;    // Trap flag
        unsigned int SF : 1;    // Sign flag
        unsigned int ZF : 1;    // Zero flag
        unsigned int U2 : 1;    // Unused field
        unsigned int AF : 1;    // Auxiliary (decimal) carry flag
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

uint32_t get_addr(uint16_t segment_reg, uint16_t addr) {
    uint32_t ret_val = segment_reg;
    ret_val <<= 4;
    return ret_val + addr;
}

#define INVALID_INSTRUCTION while(1)

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
    return byte & 0x02;
}

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

uint8_t get_io_8bit(uint32_t addr) {
    return IO_SPACE[addr];
}

uint8_t set_io_8bit(uint32_t addr, uint8_t value) {
    IO_SPACE[addr] = value;
    return 1;
}

uint16_t get_io_16bit(uint32_t addr) {
    return IO_SPACE[addr];
}

uint8_t set_io_16bit(uint32_t addr, uint16_t value) {
    IO_SPACE[addr] = value;
    return 1;
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

uint32_t segment_override(register_t segment) {
    // Use the given segment instead of standard to calculate the address in the next instruction
    // https://www.includehelp.com/embedded-system/segment-override-prefix-8086-microprocessor.aspx
    return 1;
}

void inc_register(uint8_t opcode) {
    opcode &= 0x0F;
    switch(opcode) {
        case 0x00:  // INC AX
            registers.AX ++;
            break;
        case 0x01:  // INC CX
            registers.CX ++;
            break;
        case 0x02:  // INC DX
            registers.DX ++;
            break;
        case 0x03:  // INC BX
            registers.BX ++;
            break;
        case 0x04:  // INC SP
            registers.SP ++;
            break;
        case 0x05:  // INC BP
            registers.BP ++;
            break;
        case 0x06:  // INC SI
            registers.SI ++;
            break;
        case 0x07:  // INC DI
            registers.DI ++;
            break;
    }
}

void dec_register(uint8_t opcode) {
    opcode -= 8;
    opcode &= 0x0F;
    switch(opcode) {
        case 0x00:  // DEC AX
            registers.AX --;
            break;
        case 0x01:  // DEC CX
            registers.CX --;
            break;
        case 0x02:  // DEC DX
            registers.DX --;
            break;
        case 0x03:  // DEC BX
            registers.BX --;
            break;
        case 0x04:  // DEC SP
            registers.SP --;
            break;
        case 0x05:  // DEC BP
            registers.BP --;
            break;
        case 0x06:  // DEC SI
            registers.SI --;
            break;
        case 0x07:  // DEC DI
            registers.DI --;
            break;
    }
}

uint8_t and_op(uint8_t opcode, uint8_t *data) {
    return 1;
}

uint8_t sub_op(uint8_t opcode, uint8_t *data) {
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

void push_16b(uint16_t value) {
    registers.SP -= 2;
    uint16_t *memory = (uint16_t *)MEMORY;
    memory[get_addr(registers.SS, registers.SP)] = value;
    return;
}

void push_reg_op(opcode) {
    switch(opcode) {
        case 0x06:  // PUSH ES
            push_16b(registers.ES);
            break;
        case 0x0E:  // PUSH CS
            push_16b(registers.CS);
            break;
        case 0x16:  // PUSH SS
            push_16b(registers.SS);
            break;
        case 0x1E:  // PUSH DS
            push_16b(registers.DS);
            break;
        case 0x50:  // PUSH AX
            push_16b(registers.AX);
            break;
        case 0x51:  // PUSH CX
            push_16b(registers.CX);
            break;
        case 0x52:  // PUSH DX
            push_16b(registers.DX);
            break;
        case 0x53:  // PUSH BX
            push_16b(registers.BX);
            break;
        case 0x54:  // PUSH SP
            push_16b(registers.SP);
            break;
        case 0x55:  // PUSH BP
            push_16b(registers.BP);
            break;
        case 0x56:  // PUSH SI
            push_16b(registers.SI);
            break;
        case 0x57:  // PUSH DI
            push_16b(registers.DI);
            break;
    }
}

uint8_t pop_reg_op(register_t reg) {
    return 1;
}

uint8_t push_val_op(uint16_t val) {
    return 1;
}

uint8_t add_op(uint8_t *memory) {
    return 1;
}

uint8_t or_op(uint8_t *memory) {
    return 1;
}

uint8_t adc_op(uint8_t *memory) {
    // ADD with carry
    return 1;
}

uint8_t sbb_op(uint8_t *memory) {
    // Substract with borrow
    return 1;
}

void daa_op(void) {
    // https://www.phatcode.net/res/223/files/html/Chapter_6/CH06-2.html
    // if ( (AL and 0Fh) > 9 or (AuxC = 1)) then
    //         al := al + 6
    //         AuxC := 1               ;Set Auxilliary carry.
    // endif
    // if ( (al > 9Fh) or (Carry = 1)) then
    //         al := al + 60h
    //         Carry := 1;             ;Set carry flag.
    // endif
    uint8_t al = get_l(registers.AX);
    if((al & 0x0F) > 9 || registers.flags.AF == 1) {
        al += 6;
        registers.flags.AF = 1;
    }
    if((al > 0x9F) || registers.flags.CF == 1) {
        al += 0x60;
        registers.flags.CF = 1;
    } else {
        registers.flags.CF = 0;
    }
    registers.AX = set_l(registers.AX, al);
}

void das_op(void) {
    // if ( (al and 0Fh) > 9 or (AuxC = 1)) then
    //         al := al -6
    //         AuxC = 1
    // endif
    // if (al > 9Fh or Carry = 1) then
    //         al := al - 60h
    //         Carry := 1              ;Set the Carry flag.
    // endif
    uint8_t al = get_l(registers.AX);
    if((al & 0x0F) > 9 || registers.flags.AF) {
        al -= 6;
        registers.flags.AF = 1;
    }
    if((al > 0x9F) || registers.flags.CF) {
        al -= 0x60;
        registers.flags.CF = 1;
    } else {
        registers.flags.CF = 0;
    }
    registers.AX = set_l(registers.AX, al);
}

void aaa_op(void) {
    //  If AX contains OOFFh, executing AAA on an 8088 will leave AX=0105h. On an 80386, the same operation will leave AX=0205h.
    // if ( (al and 0Fh) > 9 or (AuxC =1) ) then
    //         if (8088 or 8086) then 
    //                 al := al + 6
    //         else 
    //                 ax := ax + 6
    //         endif
    //         ah := ah + 1
    //         AuxC := 1               ;Set auxilliary carry
    //         Carry := 1              ; and carry flags.
    // else
    //         AuxC := 0               ;Clear auxilliary carry
    //         Carry := 0              ; and carry flags.
    // endif
    // al := al and 0Fh
    uint8_t al = get_l(registers.AX);
    if((al & 0x0F) > 9 || registers.flags.AF == 1) {
        al += 6;
        registers.AX += 0x0100;
        registers.flags.AF = 1;
        registers.flags.CF = 1;
    } else {
        registers.flags.AF = 0;
        registers.flags.CF = 0;
    }
    registers.AX = set_l(registers.AX, al & 0x0F);
}

void aas_op(void) {
    // if ( (al and 0Fh) > 9 or AuxC = 1) then
    //         al := al - 6
    //         ah := ah - 1
    //         AuxC := 1       ;Set auxilliary carry
    //         Carry := 1      ; and carry flags.
    // else
    //         AuxC := 0       ;Clear Auxilliary carry
    //         Carry := 0      ; and carry flags.
    // endif
    // al := al and 0Fh
    uint8_t al = get_l(registers.AX);
    uint8_t ah = get_h(registers.AX);
    if((al & 0x0F) > 9 || registers.flags.AF == 1) {
        al -= 6;
        ah -= 1;
        registers.flags.AF = 1;
        registers.flags.CF = 1;
    } else {
        registers.flags.AF = 0;
        registers.flags.CF = 0;
    }
    registers.AX = set_h(registers.AX, ah);
    registers.AX = set_l(registers.AX, al & 0x0F);
}

uint8_t xor_op(uint8_t *memory) {
    return 1;
}

uint8_t cmp_op(uint8_t *memory) {
    return 1;
}

void process_instruction(void) {
    switch(MEMORY[registers.IP]) {
        case 0x00:  // ADD REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x01:  // ADD REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x02:  // ADD REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x03:  // ADD REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x04:  // ADD AL, IMMED8           [DATA-8]
        case 0x05:  // ADD AX, immed16          [DATA-LO, DATA-HI]
            registers.IP += add_op(&MEMORY[registers.IP]);
            break;
        case 0x06:  // PUSH ES
            push_reg_op(MEMORY[registers.IP]);
            registers.IP += 1;
            break;
        case 0x07:  // POP ES
            pop_reg_op(ES);
            registers.IP += 1;
            break;
        case 0x08:  // OR REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x09:  // OR REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x0A:  // OR REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x0B:  // OR REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x0C:  // OR AL, IMMED8           [DATA-8]
        case 0x0D:  // OR AX, immed16          [DATA-LO, DATA-HI]
            registers.IP += or_op(&MEMORY[registers.IP]);
            break;
        case 0x0E:  // PUSH CS
            push_reg_op(MEMORY[registers.IP]);
            registers.IP += 1;
            break;
        case 0x0F:  // NOT USED
            INVALID_INSTRUCTION;
            break;
        case 0x10:  // ADC REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x11:  // ADC REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x12:  // ADC REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x13:  // ADC REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x14:  // ADC AL, IMMED8           [DATA-8]
        case 0x15:  // ADC AX, immed16          [DATA-LO, DATA-HI]
            registers.IP += adc_op(&MEMORY[registers.IP]);
            break;
        case 0x16:  // PUSH SS
            push_reg_op(MEMORY[registers.IP]);
            registers.IP += 1;
            break;
        case 0x17:  // POP SS
            pop_reg_op(SS);
            registers.IP += 1;
            break;
        case 0x18:  // SBB REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x19:  // SBB REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x1A:  // SBB REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x1B:  // SBB REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x1C:  // SBB AL, IMMED8           [DATA-8]
        case 0x1D:  // SBB AX, immed16          [DATA-LO, DATA-HI]
            registers.IP += sbb_op(&MEMORY[registers.IP]);
            break;
        case 0x1E:  // PUSH DS
            push_reg_op(MEMORY[registers.IP]);
            registers.IP += 1;
            break;
        case 0x1F:  // POP DS
            pop_reg_op(DS);
            registers.IP += 1;
            break;
        case 0x20:  // AND REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x21:  // AND REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x22:  // AND REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x23:  // AND REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x24:  // AND AL, IMMED8           [DATA-8]
        case 0x25:  // AND AX, immed16          [DATA-LO, DATA-HI]
            registers.IP += and_op(&MEMORY[registers.IP]);
            break;
        case 0x26:  // ES:
            segment_override(ES);  // ES: segment overrige prefix
            registers.IP += 1;
            break;
        case 0x27:  // DAA
            daa_op();
            registers.IP += 1;
            break;
        case 0x28:  // SUB REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x29:  // SUB REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x2A:  // SUB REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x2B:  // SUB REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x2C:  // SUB AL, IMMED8           [DATA-8]
        case 0x2D:  // SUB AX, immed16          [DATA-LO, DATA-HI]
            registers.IP += sub_op(&MEMORY[registers.IP]);
            break;
        case 0x2E:  // CS:
            segment_override(CS);  // CS: segment overrige prefix
            registers.IP += 1;
            break;
        case 0x2F:  // DAS
            das_op();
            registers.IP += 1;
            break;
        case 0x30:  // XOR REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x31:  // XOR REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x32:  // XOR REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x33:  // XOR REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x34:  // XOR AL, IMMED8           [DATA-8]
        case 0x35:  // XOR AX, immed16          [DATA-LO, DATA-HI]
            registers.IP += xor_op(&MEMORY[registers.IP]);
            break;
        case 0x36:  // SS:
            segment_override(SS);  // SS: segment overrige prefix
            registers.IP += 1;
            break;
        case 0x37:  // AAA
            aaa_op();
            registers.IP += 1;
            break;
        case 0x38:  // CMP REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x39:  // CMP REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x3A:  // CMP REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x3B:  // CMP REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x3C:  // CMP AL, IMMED8           [DATA-8]
        case 0x3D:  // CMP AX, immed16          [DATA-LO, DATA-HI]
            registers.IP += cmp_op(&MEMORY[registers.IP]);
            break;
        case 0x3E:  // DS:
            segment_override(DS);  // DS: segment overrige prefix
            registers.IP += 1;
            break;
        case 0x3F:  // AAS
            aas_op();
            registers.IP += 1;
            break;
        case 0x40:  // INC AX
        case 0x41:  // INC CX
        case 0x42:  // INC DX
        case 0x43:  // INC BX
        case 0x44:  // INC SP
        case 0x45:  // INC BP
        case 0x46:  // INC SI
        case 0x47:  // INC DI
            inc_register(&MEMORY[registers.IP]);
            registers.IP += 1;
            break;
        case 0x48:  // DEC AX
        case 0x49:  // DEC CX
        case 0x4A:  // DEC DX
        case 0x4B:  // DEC BX
        case 0x4C:  // DEC SP
        case 0x4D:  // DEC BP
        case 0x4E:  // DEC SI
        case 0x4F:  // DEC DI
            dec_register(&MEMORY[registers.IP]);
            registers.IP += 1;
            break;
        case 0x50:  // PUSH AX
        case 0x51:  // PUSH CX
        case 0x52:  // PUSH DX
        case 0x53:  // PUSH BX
        case 0x54:  // PUSH SP
        case 0x55:  // PUSH BP
        case 0x56:  // PUSH SI
        case 0x57:  // PUSH DI
            push_reg_op(&MEMORY[registers.IP]);
            registers.IP += 1;
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
                    registers.IP += rol_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI
                    break;
                case 1: // ROR REG8/MEM8, 1
                    registers.IP += ror_op(memory[1], &memory[2]);  // MOD 001 R/M, DISP-LO, DISP-HI
                    break;
                case 2: // RCL REG8/MEM8, 1
                    registers.IP += rcl_op(memory[1], &memory[2]);  // MOD 010 R/M, DISP-LO, DISP-HI
                    break;
                case 3: // RCR REG8/MEM8, 1
                    registers.IP += rcr_op(memory[1], &memory[2]);  // MOD 011 R/M, DISP-LO, DISP-HI
                    break;
                case 4: // SAL/SHL REG8/MEM8, 1
                    registers.IP += shl_op(memory[1], &memory[2]);  // MOD 100 R/M, DISP-LO, DISP-HI
                    break;
                case 5: // SHR REG8/MEM8, 1
                    registers.IP += shr_op(memory[1], &memory[2]);  // MOD 101 R/M, DISP-LO, DISP-HI
                    break;
                case 6:
                    INVALID_INSTRUCTION;    // MOD 110 R/M,
                    break;
                case 7: // SAR REG8/MEM8, 1
                    registers.IP += sar_op(memory[1], &memory[2]);  // MOD 111 R/M, DISP-LO, DISP-HI
                    break;
            }
            break;
        case 0xD1:
            // 16-bit operations
            switch(get_mode_field(memory[1])) {
                case 0: // ROL REG16/MEM16, 1
                    registers.IP += rol_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI
                    break;
                case 1: // ROR REG16/MEM16, 1
                    registers.IP += ror_op(memory[1], &memory[2]);  // MOD 001 R/M, DISP-LO, DISP-HI
                    break;
                case 2: // RCL REG16/MEM16, 1
                    registers.IP += rcl_op(memory[1], &memory[2]);  // MOD 010 R/M, DISP-LO, DISP-HI
                    break;
                case 3: // RCR REG16/MEM16, 1
                    registers.IP += rcr_op(memory[1], &memory[2]);  // MOD 01 R/M, DISP-LO, DISP-HI
                    break;
                case 4: // SAL/SHL REG16/MEM16, 1
                    registers.IP += shl_op(memory[1], &memory[2]);  // MOD 100 R/M, DISP-LO, DISP-HI
                    break;
                case 5: // SHR REG16/MEM16, 1
                    registers.IP += shr_op(memory[1], &memory[2]);  // MOD 101 R/M, DISP-LO, DISP-HI
                    break;
                case 6:
                    INVALID_INSTRUCTION;    // MOD 110 R/M,
                    break;
                case 7: // SAR REG16/MEM16, 1
                    registers.IP += sar_op(memory[1], &memory[2]);  // MOD 111 R/M, DISP-LO, DISP-HI
                    break;
            }
            break;
        case 0xD2:
            // 8-bit operations
            switch(get_mode_field(memory[1])) {
                case 0: // ROL REG8/MEM8, CL
                    registers.IP += rol_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI
                    break;
                case 1: // ROR REG8/MEM8, CL
                    registers.IP += ror_op(memory[1], &memory[2]);  // MOD 001 R/M, DISP-LO, DISP-HI
                    break;
                case 2: // RCL REG8/MEM8, CL
                    registers.IP += rcl_op(memory[1], &memory[2]);  // MOD 010 R/M, DISP-LO, DISP-HI
                    break;
                case 3: // RCR REG8/MEM8, CL
                    registers.IP += rcr_op(memory[1], &memory[2]);  // MOD 011 R/M, DISP-LO, DISP-HI
                    break;
                case 4: // SAL/SHL REG8/MEM8, CL
                    registers.IP += shl_op(memory[1], &memory[2]);  // MOD 100 R/M, DISP-LO, DISP-HI
                    break;
                case 5: // SHR REG8/MEM8, CL
                    registers.IP += shr_op(memory[1], &memory[2]);  // MOD 101 R/M, DISP-LO, DISP-HI
                    break;
                case 6:
                    INVALID_INSTRUCTION;    // MOD 110 R/M,
                    break;
                case 7: // SAR REG8/MEM8, CL
                    registers.IP += sar_op(memory[1], &memory[2]);  // MOD 111 R/M, DISP-LO, DISP-HI
                    break;
            }
            break;
        case 0xD3:
            // 16-bit operations
            switch(get_mode_field(memory[1])) {
                case 0: // ROL REG16/MEM16, CL
                    registers.IP += rol_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI
                    break;
                case 1: // ROR REG16/MEM16, CL
                    registers.IP += ror_op(memory[1], &memory[2]);  // MOD 001 R/M, DISP-LO, DISP-HI
                    break;
                case 2: // RCL REG16/MEM16, CL
                    registers.IP += rcl_op(memory[1], &memory[2]);  // MOD 010 R/M, DISP-LO, DISP-HI
                    break;
                case 3: // RCR REG16/MEM16, CL
                    registers.IP += rcr_op(memory[1], &memory[2]);  // MOD 011 R/M, DISP-LO, DISP-HI
                    break;
                case 4: // SAL/SHL REG16/MEM16, CL
                    registers.IP += shl_op(memory[1], &memory[2]);  // MOD 100 R/M, DISP-LO, DISP-HI
                    break;
                case 5: // SHR REG16/MEM16, CL
                    registers.IP += shr_op(memory[1], &memory[2]);  // MOD 101 R/M, DISP-LO, DISP-HI
                    break;
                case 6:
                    INVALID_INSTRUCTION;    // MOD 110 R/M,
                    break;
                case 7: // SAR REG16/MEM16, CL
                    registers.IP += sar_op(memory[1], &memory[2]);  // MOD 111 R/M, DISP-LO, DISP-HI
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
            registers.IP += xlat_op();
            break;
        case 0xD8:  // ESC OPCODE, SOURCE
        case 0xD9:
        case 0xDA:
        case 0xDB:
        case 0xDC:
        case 0xDD:
        case 0xDE:
        case 0xDF:
            if((memory[0] == 0xD8 && get_register_field(memory[1]) == 0) ||
               (memory[0] == 0xDF && get_register_field(memory[1]) == 0x07)) {
                registers.IP += esc_op();   // MOD 000/111 R/M; TODO: Not sure
            } else {
                registers.IP += esc_op(memory[1], &memory[2]);  // MOD 000/111 R/M, DISP-LO, DISP-HI
            }
            break;
        case 0xE0:  // LOOPNE/LOOPNZ SHORT-LABEL
            // LOOPNE decrements ecx and checks that ecx is not zero and ZF is not set - if these
            // conditions are met, it jumps at label, otherwise falls through
            registers.CX--;
            if(registers.CX > 0 && registers.flags.ZF == 0) {
                registers.IP += memory[1];  // IP-INC-8
            } else {
                registers.IP += 2;
            }
            break;
        case 0xE1:  // LOOPE/LOOPZ SHORT-LABEL
            // LOOPE decrements ecx and checks that ecx is not zero and ZF is set - if these
            // conditions are met, it jumps at label, otherwise falls through
            registers.CX--;
            if(registers.CX > 0 && registers.flags.ZF) {
                registers.IP += memory[1];  // IP-INC-8
            } else {
                registers.IP += 2;
            }
            break;
        case 0xE2:  // LOOP SHORT-LABEL
            // LOOP decrements ecx and checks if ecx is not zero, if that 
            // condition is met it jumps at specified label, otherwise falls through
            registers.CX--;
            if(registers.CX > 0) {
                registers.IP += memory[1];  // IP-INC-8
            } else {
                registers.IP += 2;
            }
            break;
        case 0xE3:  // JPXZ
            // JCXZ (Jump if ECX Zero) branches to the label specified in the instruction if it finds a value of zero in ECX
            if(registers.CX == 0) {
                registers.IP += memory[1];  // IP-INC-8
            } else {
                registers.IP += 2;
            }
            break;
        case 0xE4:  // IN AL, IMMED8
            registers.AX = set_l(registers.AX, get_io_8bit(memory[1]));  // DATA-8
            registers.IP += 2;
            break;
        case 0xE5:  // IN AX, IMMED8
            registers.AX = get_io_16bit(memory[1]);  // DATA-8
            registers.IP += 2;
            break;
        case 0xE6:  // OUT AL, IMMED8
            set_io_8bit(memory[1], get_l(registers.AX)); // DATA-8
            registers.IP += 2;
            break;
        case 0xE7:  // OUT AX, IMMED8
            set_io_16bit(memory[1], registers.AX); // DATA-8
            registers.IP += 2;
            break;
        case 0xE8: {  // CALL NEAR-PROC
            uint16_t addr = memory[2];
            addr <<= 8;
            addr += memory[1];
            registers.IP += addr;   // IP-INC-LO, IP-INC-HI
            break;
        }
        case 0xE9: {  // JMP NEAR-LABEL
            uint16_t addr = memory[2];
            addr <<= 8;
            addr += memory[1];
            registers.IP += addr;   // IP-INC-LO, IP-INC-HI
            break;
        }
        case 0xEA: {  // JMP FAR-LABEL
            uint16_t addr = memory[4];
            addr <<= 8;
            addr += memory[3];
            registers.CS = addr;
            addr = memory[2];
            addr <<= 8;
            addr += memory[1];
            registers.IP += addr;   // IP-LO, IP-HI, CS-LO, CS-HI
            break;
        }
        case 0xEB:  // JMP SHORT-LABEL
            registers.IP += memory[1];
            break;
        case 0xEC:  // IN AL, DX
            registers.AX = set_l(registers.AX, get_io_8bit(registers.DX));
            registers.IP += 1;
            break;
        case 0xED:  // IN AX, DX
            registers.AX = get_io_16bit(registers.DX);  // DATA-8
            registers.IP += 1;
            break;
        case 0xEE:  // OUT AL, DX
            set_io_8bit(registers.DX, get_l(registers.AX)); // DATA-8
            registers.IP += 2;
            break;
        case 0xEF:  // OUT AX, DX
            set_io_16bit(registers.DX, registers.AX); // DATA-8
            registers.IP += 1;
            break;
        case 0xF0:  // LOCK (prefix)
            assert_LOCK_pin();
            registers.IP += 1;
            break;
        case 0xF1:
            INVALID_INSTRUCTION;
            break;
        case 0xF2:  // REPNE/REPNZ
            // while(registers.CX != 0) {
            //     execute_next_command();
            //     registers.CX --;
            // }
            break;
        case 0xF3:  // REP/REPE/REPZ
            // while(registers.CX != 0) {
            //     execute_next_command();
            //     registers.CX --;
            // }
            break;
        case 0xF4:  // HLT
            // halts the CPU until the next external interrupt is fired
            break;
        case 0xF5:  // CMC
            // Inverts the CF flag
            if(registers.flags.CF) {
                registers.flags.CF = 0;
            } else {
                registers.flags.CF = 1;
            }
            registers.IP += 1;
            break;
        case 0xF6:
            switch(get_mode_field(memory[1])) {
                case 0: // TEST REG8/MEM8, IMMED8
                    // TEST sets the zero flag, ZF, when the result of the AND operation is zero.
                    // If two operands are equal, their bitwise AND is zero when both are zero.
                    // TEST also sets the sign flag, SF, when the most significant bit is set in
                    // the result, and the parity flag, PF, when the number of set bits is even.
                    registers.IP += test_8b_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI, DATA-8
                    break;
                case 1:
                    INVALID_INSTRUCTION;
                    break;
                case 2: // NOT REG8/MEM8
                    registers.IP += not_op(memory[1], &memory[2]);  // NOT 010 R/M, DISP-LO, DISP-HI
                    break;
                case 3: // NEG REG8/MEM8
                    // subtracts the operand from zero and stores the result in the same operand.
                    // The NEG instruction affects the carry, overflow, sign, zero, and parity flags according to the result.
                    registers.IP += neg_op(memory[1], &memory[2]);  // NEG 010 R/M, DISP-LO, DISP-HI
                    break;
                case 4: // MUL REG8/MEM8
                    // the source operand (in a general-purpose register or memory location) is multiplied by the value in the AL, AX, EAX, or RAX
                    // register (depending on the operand size) and the product (twice the size of the input operand) is stored in the
                    // AX, DX:AX, EDX:EAX, or RDX:RAX registers, respectively
                    registers.IP += mul_op(memory[1], &memory[2]);  // MUL 010 R/M, DISP-LO, DISP-HI
                    break;
                case 5: // IMUL REG8/MEM8 (signed)
                    registers.IP += mul_op(memory[1], &memory[2]);  // IMUL 010 R/M, DISP-LO, DISP-HI
                    break;
                case 6: // DIV REG8/MEM8
                    registers.IP += div_op(memory[1], &memory[2]);  // DIV 010 R/M, DISP-LO, DISP-HI
                    break;
                case 7: // IDIV REG8/MEM8
                    registers.IP += idiv_op(memory[1], &memory[2]);  // IDIV 010 R/M, DISP-LO, DISP-HI
                    break;
            }
            break;
        case 0xF7:
            switch(get_mode_field(memory[1])) {
                case 0: // TEST REG16/MEM16, IMMED16
                    registers.IP += test_16b_op(memory[1], &memory[2]);  // MOD 000 R/M, DISP-LO, DISP-HI, DATA-16
                    break;
                case 1:
                    INVALID_INSTRUCTION;
                    break;
                case 2: // NOT REG16/MEM16
                    registers.IP += not_op(memory[1], &memory[2]);  // NOT 010 R/M, DISP-LO, DISP-HI
                    break;
                case 3: // NEG REG16/MEM16
                    registers.IP += neg_op(memory[1], &memory[2]);  // NEG 010 R/M, DISP-LO, DISP-HI
                    break;
                case 4: // MUL REG16/MEM16
                    registers.IP += mul_op(memory[1], &memory[2]);  // MUL 010 R/M, DISP-LO, DISP-HI
                    break;
                case 5: // IMUL REG16/MEM16 (signed)
                    registers.IP += mul_op(memory[1], &memory[2]);  // IMUL 010 R/M, DISP-LO, DISP-HI
                    break;
                case 6: // DIV REG16/MEM16
                    registers.IP += div_op(memory[1], &memory[2]);  // DIV 010 R/M, DISP-LO, DISP-HI
                    break;
                case 7: // IDIV REG16/MEM16
                    registers.IP += idiv_op(memory[1], &memory[2]);  // IDIV 010 R/M, DISP-LO, DISP-HI
                    break;
            }
            break;
        case 0xF8:  // CLC
            // Clear Carry Flag
            registers.flags.CF = 0;
            registers.IP += 1;
            break;
        case 0xF9:  // STC
            // Set Carry Flag
            registers.flags.CF = 1;
            registers.IP += 1;
            break;
        case 0xFA:  // CLI
            // Clear Interrupt Flag, causes the processor to ignore maskable external interrupts
            registers.flags.IF = 0;
            registers.IP += 1;
            break;
        case 0xFB:  // STI
            // Set Interrupt Flag
            registers.flags.IF = 1;
            registers.IP += 1;
            break;
        case 0xFC:  // CLD
            // Clear Direction Flag
            // when the direction flag is 0, the instructions work by incrementing the pointer
            // to the data after every iteration, while if the flag is 1, the pointer is decremented
            registers.flags.DF = 0;
            registers.IP += 1;
            break;
        case 0xFD:  // CLD
            // Set Direction Flag
            registers.flags.DF = 1;
            registers.IP += 1;
            break;
        case 0xFE:
            break;
        case 0xFF:
            break;

        default:
            INVALID_INSTRUCTION;
    }
}
