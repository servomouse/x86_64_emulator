// https://en.wikipedia.org/wiki/Intel_8086
#include "8086_cpu.h"
#include "wires.h"
// #include "8086_io.h"
// #include "8086_mem.h"
// #include "8253_timer.h"
#include <string.h>

#define COMMON_LOG_FILE "logs/cpu_log.txt"
#define REGISTERS_FILE  "logs/regs.txt"
#define CPU_DUMP_FILE  "data/cpu_regs.bin"

WRITE_FUNC_PTR(mem_write);
READ_FUNC_PTR(mem_read);
WRITE_FUNC_PTR(io_write);
READ_FUNC_PTR(io_read);
uint16_t(*code_read)(uint32_t, uint8_t);

typedef enum {
    invalid_register = 0,
    AX_register,
    AL_register,
    AH_register,
    BX_register,
    BL_register,
    BH_register,
    CX_register,
    CL_register,
    CH_register,
    DX_register,
    DL_register,
    DH_register,
    SI_register,
    DI_register,
    BP_register,
    SP_register,
    IP_register,
    CS_register,
    DS_register,
    ES_register,
    SS_register,
    FLAGS_register,
    override_segment,
} register_name_t;

typedef struct {
    // Helper registers
    uint32_t ticks;
    uint32_t invalid_operations;
    uint8_t halt;
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
    uint16_t IP;    // Instruction pointer
    // Segment registers
    uint16_t CS;    // Sode segment
    uint16_t DS;    // Data segment
    uint16_t ES;    // Extra segment
    uint16_t SS;    // Stack segment
    // Status register
    uint16_t flags;
    uint16_t int_vector;    // 0xFFFF is invalid value
    uint16_t prefixes;
    register_name_t override_segment;
} registers_t;
registers_t *REGS = NULL;

// register_name_t override_segment = invalid_register;

typedef enum {
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

typedef enum {
    REPE = 0,   // REPE/REPZ prefix
    REPNE,      // REPNE/REPNZ prefix
} prefix_t;

typedef enum {
    LOGIC_OP = 0,
    SHIFT_R_OP,
    SHIFT_L_OP,
    ADD_OP,
    SUB_OP,
    MUL_OP,
    DIV_OP,
} operation_t;

typedef struct {
    union {
        register_name_t register_name;
        uint32_t address;
    } src;
    union {
        register_name_t register_name;
        uint32_t address;
    } dst;
    int16_t src_val;
    int16_t dst_val;
    uint8_t width;      // operand width in bytes: 1 or 2
    uint8_t src_type;   // 0 - register, 1 - memory
    uint8_t dst_type;   // 0 - register, 1 - memory
    uint8_t num_bytes;  // How many bytes does instruction take
    char * source;
    char * destination;
} operands_t;

uint32_t processed_commands[0xFF];

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

uint8_t get_prefix(prefix_t px) {
    return (REGS->prefixes & (1 << px)) > 0;
}

void set_prefix(prefix_t px, uint16_t value) {
    uint16_t mask = 1 << px;
    if (value > 0) {
        REGS->prefixes |= mask;
    } else {
        REGS->prefixes &= ~mask;
    }
}

uint8_t get_parity(uint16_t byte) {
    // Returns 1 if the byte contains an even number of 1-bits, otherwise returns 0
    uint8_t counter = 0;
    for(int i=1; i<0x100; i<<=1) {
        if ((byte & i) > 0)
            counter++;
    }
    return (counter & 0x01)? 0:1;
}

void update_flags(uint32_t dst, uint32_t src, uint32_t res, uint8_t width, operation_t op_type) {
    if(width == 1) {
        set_flag(SF, (res & 0x80) > 0);
        set_flag(ZF, (res & 0xFF) == 0);
        set_flag(PF, get_parity(res & 0xFF));
    } else if (width == 2) {
        set_flag(SF, (res & 0x8000) > 0);
        set_flag(ZF, (res & 0xFFFF) == 0);
        set_flag(PF, get_parity(res & 0xFFFF));
    }
    switch(op_type) {
        case LOGIC_OP: {
            set_flag(CF, 0);    // Always 0 for logic operations
            //AF is not affected
            set_flag(OF, 0);    // Always 0 for logic operations
            break;
        }
        case SHIFT_L_OP: {
            if(width == 1) {
                set_flag(CF, (dst & 0x80) != 0);
            } else if (width == 2) {
                set_flag(CF, (dst & 0x8000) != 0);
            }
            // Does not affect AF
            if(src == 1) {   // Affects OF only for single-bit shift
                if(width == 1) {
                    set_flag(OF, ((dst ^ res) & 0x80) != 0);
                } else if (width == 2) {
                    set_flag(OF, ((dst ^ res) & 0x8000) != 0);
                }
            }
            break;
        }
        case SHIFT_R_OP: {
            if(width == 1) {
                set_flag(CF, (dst & 1) != 0);
            } else if (width == 2) {
                set_flag(CF, (dst & 1) != 0);
            }
            // Does not affect AF
            if(src == 1) {   // Affects OF only for single-bit shift
                if(width == 1) {
                    set_flag(OF, ((dst ^ res) & 0x80) != 0);
                } else if (width == 2) {
                    set_flag(OF, ((dst ^ res) & 0x8000) != 0);
                }
            }
            break;
        }
        case ADD_OP: {
            if(width == 1) {
                set_flag(CF, (res & 0xFF) < (dst & 0xFF));
            } else if (width == 2) {
                set_flag(CF, (res & 0xFFFF) < (dst & 0xFFFF));
            }
            if(width == 1) {
                set_flag(AF, ((dst & 0x0F) + (src & 0x0F)) > 0x0F);
            } else if (width == 2) {
                set_flag(AF, ((dst & 0x0F) + (src & 0x0F)) > 0x0F);
            }
            if(width == 1) {
                set_flag(OF, ((dst & 0x7F) + (src & 0x7F)) > 0x7F);
            } else if (width == 2) {
                set_flag(OF, ((dst & 0x7FFF) + (src & 0x7FFF)) > 0x7FFF);
            }
            break;
        }
        case SUB_OP: {
            if(width == 1) {
                set_flag(CF, (dst & 0xFF) < (src & 0xFF));
            } else if (width == 2) {
                set_flag(CF, (dst & 0xFFFF) < (src & 0xFFFF));
            }
            if(width == 1) {
                set_flag(AF, (dst & 0x0F) < (src & 0x0F));
            } else if (width == 2) {
                set_flag(AF, (dst & 0x0F) < (src & 0x0F));
            }
            if(width == 1) {
                set_flag(OF, (dst & 0x7F) < (src & 0x7F));
            } else if (width == 2) {
                set_flag(OF, (dst & 0x7FFF) < (src & 0x7FFF));
            }
            break;
        }
        case MUL_OP: {
            if(width == 1) {
                set_flag(CF, (res & 0xFF00) != 0);
            } else if (width == 2) {
                set_flag(CF, (res & 0xFFFF0000) != 0);
            }
            // AF is not affected
            if(width == 1) {    // The same as for CF
                set_flag(OF, (res & 0xFF00) != 0);
            } else if (width == 2) {
                set_flag(OF, (res & 0xFFFF0000) != 0);
            }
            break;
        }
        case DIV_OP: {
            // Does not affect CF, AF and OF
            break;
        }
        default:
            printf("ERROR: Incorrect operation type: %d", op_type);
    }
}

uint8_t get_mode_field(uint8_t byte) {
    // 00 -> Memory mode, no displacement follows (except when reg_mem_field == 110, then 16-bit displacement follows)
    // 01 -> Memory mode, 8-bit displacement follows
    // 10 -> Memory mode, 16-bit displacement follows
    // 11 -> Register mode, no displacement
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
    return (byte & (~0xC7)) >> 3;
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

char * get_reg_name_string(register_name_t reg_name) {
    switch (reg_name) {
        case invalid_register:
            return "Invalid Register";
        case AX_register:
            return "AX";
        case AL_register:
            return "AL";
        case AH_register:
            return "AH";
        case BX_register:
            return "BX";
        case BL_register:
            return "BL";
        case BH_register:
            return "BH";
        case CX_register:
            return "CX";
        case CL_register:
            return "CL";
        case CH_register:
            return "CH";
        case DX_register:
            return "DX";
        case DL_register:
            return "DL";
        case DH_register:
            return "DH";
        case SI_register:
            return "SI";
        case DI_register:
            return "DI";
        case BP_register:
            return "BP";
        case SP_register:
            return "SP";
        case IP_register:
            return "IP";
        case CS_register:
            return "CS";
        case DS_register:
            return "DS";
        case ES_register:
            return "ES";
        case SS_register:
            return "SS";
        default:
            return "Invalid Register";
    }
}

// Width == 1 for byte and 2 for word
register_name_t get_reg_name(uint8_t rm_field, uint8_t width) {
    if((width < 1) || (width > 2)) {
        printf("ERROR: Invalid width: %d\n", width);
    }
    switch(rm_field) {
        case 0:
            if(width == 1)
                return AL_register;
            else
                return AX_register;
            break;
        case 1:
            if(width == 1)
                return CL_register;
            else
                return CX_register;
            break;
        case 2:
            if(width == 1)
                return DL_register;
            else
                return DX_register;
            break;
        case 3:
            if(width == 1)
                return BL_register;
            else
                return BX_register;
            break;
        case 4:
            if(width == 1)
                return AH_register;
            else
                return SP_register;
            break;
        case 5:
            if(width == 1)
                return CH_register;
            else
                return BP_register;
            break;
        case 6:
            if(width == 1)
                return DH_register;
            else
                return SI_register;
            break;
        case 7:
            if(width == 1)
                return BH_register;
            else
                return DI_register;
            break;
        default:
            return invalid_register;
    }
}

uint16_t get_register_value(register_name_t reg_name) {
    switch(reg_name) {
        case AX_register:
            return REGS->AX;
        case AL_register:
            return get_l(REGS->AX);
        case AH_register:
            return get_h(REGS->AX);
        case BX_register:
            return REGS->BX;
        case BL_register:
            return get_l(REGS->BX);
        case BH_register:
            return get_h(REGS->BX);
        case CX_register:
            return REGS->CX;
        case CL_register:
            return get_l(REGS->CX);
        case CH_register:
            return get_h(REGS->CX);
        case DX_register:
            return REGS->DX;
        case DL_register:
            return get_l(REGS->DX);
        case DH_register:
            return get_h(REGS->DX);
        case SI_register:
            return REGS->SI;
        case DI_register:
            return REGS->DI;
        case BP_register:
            return REGS->BP;
        case SP_register:
            return REGS->SP;
        case IP_register:
            return REGS->IP;
        case CS_register:
            return REGS->CS;
        case DS_register:
            return REGS->DS;
        case ES_register:
            return REGS->ES;
        case SS_register:
            return REGS->SS;
        case override_segment:
            return REGS->override_segment;
        default:
            mylog("logs/main.log", "Invalid register name: 0x%02X\n", reg_name);
            return 0;
    }
}

void set_register_value(register_name_t reg_name, uint16_t value) {
    switch(reg_name) {
        case AX_register:
            REGS->AX = value;
            break;
        case AL_register:
            REGS->AX = set_l(REGS->AX, value & 0xFF);
            break;
        case AH_register:
            REGS->AX = set_h(REGS->AX, value & 0xFF);
            break;
        case BX_register:
            REGS->BX = value;
            break;
        case BL_register:
            REGS->BX = set_l(REGS->BX, value & 0xFF);
            break;
        case BH_register:
            REGS->BX = set_h(REGS->BX, value & 0xFF);
            break;
        case CX_register:
            REGS->CX = value;
            break;
        case CL_register:
            REGS->CX = set_l(REGS->CX, value & 0xFF);
            break;
        case CH_register:
            REGS->CX = set_h(REGS->CX, value & 0xFF);
            break;
        case DX_register:
            REGS->DX = value;
            break;
        case DL_register:
            REGS->DX = set_l(REGS->DX, value & 0xFF);
            break;
        case DH_register:
            REGS->DX = set_h(REGS->DX, value & 0xFF);
            break;
        case SI_register:
            REGS->SI = value;
            break;
        case DI_register:
            REGS->DI = value;
            break;
        case BP_register:
            REGS->BP = value;
            break;
        case SP_register:
            REGS->SP = value;
            break;
        case IP_register:
            REGS->IP = value;
            break;
        case CS_register:
            REGS->CS = value;
            break;
        case DS_register:
            REGS->DS = value;
            break;
        case ES_register:
            REGS->ES = value;
            break;
        case SS_register:
            REGS->SS = value;
            break;
        case FLAGS_register:
            REGS->flags = value;
            break;
        case override_segment:
            REGS->override_segment = value;
            break;
        default:
            mylog("logs/main.log", "Invalid register name: 0x%02X\n", reg_name);
    }
}

uint32_t get_addr(register_name_t segment_reg, uint16_t addr) {
    // override_segment
    uint32_t ret_val = 0;
    if(get_register_value(override_segment) != invalid_register) {
        ret_val = get_register_value(override_segment);
        set_register_value(override_segment, invalid_register);
    } else {
        ret_val = get_register_value(segment_reg);
    }
    ret_val <<= 4;
    return ret_val + addr;
}

void push_register(uint16_t value) {
    set_register_value(SP_register, get_register_value(SP_register) - 2);
    uint32_t addr = get_addr(SS_register, get_register_value(SP_register));
    mem_write(addr, value, 2);
}

void pop_register(register_name_t reg_name) {
    uint32_t addr = get_addr(SS_register, get_register_value(SP_register));
    set_register_value(reg_name, mem_read(addr, 2));
    set_register_value(SP_register, get_register_value(SP_register) + 2);
}

uint16_t pop_value(void) {
    uint32_t addr = get_addr(SS_register, get_register_value(SP_register));
    uint16_t value = mem_read(addr, 2);
    set_register_value(SP_register, get_register_value(SP_register) + 2);
    return value;
}

// Set single to 1 if the REG field is used as an opcode extenrion
operands_t decode_operands(uint8_t opcode, uint8_t *data, uint8_t single) {
    uint8_t mod_field = get_mode_field(data[0]);
    uint8_t rm_field = get_reg_mem_field(data[0]);
    uint8_t reg_field = get_register_field(data[0]);
    operands_t operands = {0};
    if((opcode & 0x01) == 0) {
        operands.width = 1;
    } else {
        operands.width = 2;
    }
    operands.num_bytes = 1;
    uint32_t addr = 0;
    switch(rm_field) {
        case 0:
            addr = get_register_value(BX_register) + get_register_value(SI_register);
            break;
        case 1:
            addr = get_register_value(BX_register) + get_register_value(DI_register);
            break;
        case 2:
            addr = get_register_value(BP_register) + get_register_value(SI_register);
            break;
        case 3:
            addr = get_register_value(BP_register) + get_register_value(DI_register);
            break;
        case 4:
            addr = get_register_value(SI_register);
            break;
        case 5:
            addr = get_register_value(DI_register);
            break;
        case 6:
            if(mod_field == 0) {
                addr = data[1] + (data[2] << 8);
                operands.num_bytes += 2;
            } else {
                addr = get_register_value(BP_register);
            }
            break;
        case 7:
            addr = get_register_value(BX_register);
            break;
    }
    if(mod_field == 1) {
        addr += data[1];
        operands.num_bytes += 1;
    } else if(mod_field == 2) {
        addr += data[1] + (data[2] << 8);
        operands.num_bytes += 2;
    }
    addr = get_addr(DS_register, addr);
    if(((opcode & 0x02) == 0) || single) {  // Destination bit == 0 or REG field is used as an opcode extension
        if(single) {
            // REG field is used as an opcode extension
            operands.src_type = 2;  // Immed
            if(operands.width == 1) {
                operands.source = "immed8";
            } else if(operands.width == 2) {
                operands.source = "immed16";
            }
        } else {
            // Instruction source is specified in REG field
            operands.src_type = 0;
            operands.src.register_name = get_reg_name(reg_field, operands.width);
            operands.source = get_reg_name_string(operands.src.register_name);
        }
        if(mod_field == 3) {    // Register_mode
            operands.dst_type = 0;
            operands.dst.register_name = get_reg_name(rm_field, operands.width);
            operands.destination = get_reg_name_string(operands.dst.register_name);
        } else {    // Memory mode
            operands.dst_type = 1;
            operands.dst.address = addr;
            operands.destination = "memory";
        }
    } else {
        // Instruction destination is specified in REG field
        operands.dst_type = 0;
        operands.dst.register_name = get_reg_name(reg_field, operands.width);
        operands.destination = get_reg_name_string(operands.dst.register_name);
        if(mod_field == 3) {    // Register_mode
            operands.src_type = 0;
            operands.src.register_name = get_reg_name(rm_field, operands.width);
            operands.source = get_reg_name_string(operands.src.register_name);
        } else {    // Memory mode
            operands.src_type = 1;
            operands.src.address = addr;
            operands.source = "memory";
        }
    }
    if(operands.src_type == 0) {    // Register mode
        operands.src_val = get_register_value(operands.src.register_name);
    } else if(operands.src_type == 1) { // Memory mode
        operands.src_val = mem_read(addr, operands.width);
    } else if(operands.src_type == 2) { // Immed mode
        if(operands.width == 1) {   // Immed8
            operands.src_val = data[operands.num_bytes];
        } else if(operands.width == 2) {    // Immed16
            operands.src_val = data[operands.num_bytes] + (data[operands.num_bytes+1] << 8);
        }
    }
    if(operands.dst_type == 0) {    // Register mode
        operands.dst_val = get_register_value(operands.dst.register_name);
    } else {                        // Memory mode
        operands.dst_val = mem_read(addr, operands.width);
    }
    return operands;
}

int16_t jmp_instr(uint8_t opcode, uint8_t *data) {
    // Conditional Jump instructions table:
    // Mnemonic Condition tested        Jump if ... (p69, 2-46)         Opcode
    // JA/JNBE  (CF OR ZF)=O            above/not below nor equal
    // JAE/JNB  CF=O                    above or equal/ not below       0x73
    // JB/JNAE  CF=1                    below / not above nor equal
    // JBE/JNA  (CF OR ZF)=1            below or equal/ not above
    // JC       CF=1                    carry
    // JE/JZ    ZF=1                    equal/zero
    // JG/JNLE  ((SF XOR OF) OR ZF)=O   greater / not less nor equal
    // JGE/JNL  (SF XOR OF)=O           greater or equal/not less
    // JL/JNGE  (SF XOR OF)=1           less/not greater nor equal
    // JLE/JNG  ((SF XOR OF) OR ZF)=1   less or equal/ not greater
    // JNC      CF=O                    not carry
    // JNE/JNZ  ZF=O                    not equal/ not zero             0x75
    // JNO      OF=O                    not overflow                    0x71
    // JNP/JPO  PF=O                    not parity / parity odd         0x7B
    // JNS      SF=O                    not sign                        0x79
    // JO       OF=1                    overflow
    // JP/JPE   PF=1                    parity / parity equal
    // JS       SF=1                    sign
    // Relative jump jump relative to the first byte of the next
    // instruction: JMP 0x00 jumps to the first byte of the next instruction
    int32_t ip_inc = 0;
    switch(opcode) {
        case 0x70: {    // JO SHORT-LABEL: [0x70, IP-INC8]
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0x70: Relative Jump JO SHORT-LABEL");
            if (get_flag(OF) == 1) {
                mylog("logs/main.log", " to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition OF == 1 didn't meet: OF = %d\n", get_flag(OF));
            }
            break;
        }
        case 0x71: {    // JNO SHORT-LABEL: [0x71, IP-INC8]
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0x71: Relative Jump JNO");
            if (get_flag(OF) == 0) {
                // int8_t increment = (int8_t)(data[0]);
                mylog("logs/main.log", " to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition OF == 0 didn't meet: OF = %d\n", get_flag(OF));
            }
            break;
        }
        case 0x72: {    // JB/JNAE/SHORT-LABEL JC: [0x72, IP-INC8] (p269, 4-30)
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0x72: Relative Jump JB/JNAE/SHORT-LABEL JC");
            if (get_flag(CF) == 1) {
                mylog("logs/main.log", " to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition CF == 1 didn't meet: CF = %d\n", get_flag(CF));
            }
            break;
        }
        case 0x73: {    // JNB/JAE SHORT-LABEL JNC: [0x73, IP-INC8] (p269, 4-30)
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0x73: Relative Jump JNB/JAE");
            if (get_flag(CF) == 0) {
                mylog("logs/main.log", " to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition CF == 0 didn't meet: CF = %d\n", get_flag(CF));
            }
            break;
        }
        case 0x74: {  // JE/JZ SHORT-LABEL: [0x74, IP-INC8]
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0x74: Relative Jump JE/JZ");
            if (get_flag(ZF) == 1) {
                mylog("logs/main.log", " to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition ZF == 1 didn't meet: ZF = %d\n", get_flag(ZF));
            }
            break;
        }
        case 0x75: {    // JNE/JNZ SHORT-LABEL: [0x75, IP-INC8] (p269, 4-30)
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0x75: Relative Jump JNE/JNZ");
            if (get_flag(ZF) == 0) {
                mylog("logs/main.log", " to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition ZF == 0 didn't meet: ZF = %d\n", get_flag(ZF));
            }
            break;
        }
        case 0x76: {  // JBE/JNA SHORT-LABEL: [0x76, IP-INC8]
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0x76: Relative Jump JBE/JNA SHORT-LABEL");
            if ((get_flag(CF) == 1) || (get_flag(ZF) == 1)) {
                mylog("logs/main.log", " to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition (CF == 1 or ZF == 1) didn't meet: ZF = %d and CF = %d\n", get_flag(ZF), get_flag(CF));
            }
            break;
        }
        case 0x78: {  // JS SHORT-LABEL: [0x78, IP-INC8]
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0x78: Relative Jump JS SHORT-LABEL");
            if (get_flag(SF) == 1) {
                mylog("logs/main.log", " to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition SF == 1 didn't meet: SF = %d\n", get_flag(SF));
            }
            break;
        }
        case 0x79: {    // JNS SHORT-LABEL: [0x79, IP-INC8] (p269, 4-30)
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0x79: Relative Jump JNS");
            if (get_flag(SF) == 0) {
                mylog("logs/main.log", " to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition SF == 0 didn't meet: SF = %d\n", get_flag(SF));
            }
            break;
        }
        case 0x7A: {  // JP/JPE SHORT-LABEL: [0x7A, IP-INC8]
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0x7A: Relative Jump JP/JPE");
            if (get_flag(PF) == 1) {
                mylog("logs/main.log", " to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition PF == 1 didn't meet: PF = %d\n", get_flag(PF));
            }
            break;
        }
        case 0x7B: {    // JNP/JPO SHORT-LABEL: [0x7B, IP-INC8] (p269, 4-30)
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0x7B: Relative Jump JNP/JPO");
            if (get_flag(PF) == 0) {
                mylog("logs/main.log", " to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition PF == 0 didn't meet: PF = %d\n", get_flag(PF));
            }
            break;
        }
        case 0x7C: {   // JL/JNGE SHORT-LABEL: [0x7F, IP-INC8] (p269, 4-30)
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0x7C: Relative Jump JL/JNGE");
            if ((get_flag(SF) ^ get_flag(OF)) > 0) {
                mylog("logs/main.log", " to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition PF == 0 didn't meet: PF = %d\n", get_flag(PF));
            }
            break;
        }
        case 0xC3: {  // RET (intrasegment)
            uint16_t sp_reg = get_register_value(SP_register);
            uint32_t addr = get_addr(SS_register, sp_reg);
            uint16_t temp = mem_read(addr, 2);
            set_register_value(SP_register, sp_reg + 2);
            set_register_value(IP_register, temp);
            mylog("logs/main.log", "Instruction 0xC3: Intrasegment RET to IP = 0x%04X\n", get_register_value(IP_register));
            break;
        }
        case 0xCF: {  // IRET
            pop_register(IP_register);
            pop_register(CS_register);
            pop_register(FLAGS_register);
            break;
        }
        case 0xE0: {  // LOOPNE/LOOPNZ SHORT-LABEL: [0xE0, IP-INC8]
            // LOOPNE decrements CX and checks that CX is not zero and ZF is clear - if these
            // conditions are met, it jumps at label, otherwise falls through
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0xE0: Relative Jump LOOPNE/LOOPNZ SHORT-LABEL: ");
            uint16_t cx_val = get_register_value(CX_register);
            set_register_value(CX_register, cx_val - 1);
            mylog("logs/main.log", "CX = 0x%04X, ZF = 0x%02X;", get_register_value(CX_register), get_flag(ZF));
            if(get_register_value(CX_register) != 0 && get_flag(ZF) == 0) {
                mylog("logs/main.log", " jump to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition (CX != 0 and ZF == 0) didn't meet: CX = 0x%04X, ZF = %d\n", get_register_value(CX_register), get_flag(ZF));
            }
            break;
        }
        case 0xE1: {  // LOOPE/LOOPZ SHORT-LABEL: [0xE1, IP-INC8]
            // LOOPE decrements ecx and checks that ecx is not zero and ZF is set - if these
            // conditions are met, it jumps at label, otherwise falls through
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0xE1: Relative Jump LOOPE/LOOPZ SHORT-LABEL: ");
            uint16_t cx_val = get_register_value(CX_register);
            set_register_value(CX_register, cx_val - 1);
            mylog("logs/main.log", "CX = 0x%04X, ZF = 0x%02X;", get_register_value(CX_register), get_flag(ZF));
            if(get_register_value(CX_register) != 0 && get_flag(ZF) == 1) {
                mylog("logs/main.log", " jump to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition (CX != 0 and ZF == 1) didn't meet: CX = 0x%04X, ZF = %d\n", get_register_value(CX_register), get_flag(ZF));
            }
            break;
        }
        case 0xE2: {  // LOOP SHORT-LABEL
            // LOOP decrements CX by 1 and transfers control
            // to the target operand if CX is not 0; otherwise the
            // instruction following LOOP is executed.
            mylog("logs/main.log", "Instruction 0xE2: Relative Jump LOOP SHORT-LABEL: ");
            ip_inc += 2;
            uint16_t dest_val = get_register_value(CX_register);
            uint16_t res_val = dest_val - 1;
            if((data[0] == 0xFE) && (res_val != 0)) {
                res_val = 0;    // shortcut useless long waiting
            }
            set_register_value(CX_register, res_val);
            mylog("logs/main.log", "CX = 0x%04X;", res_val);
            if(res_val != 0) {
                mylog("logs/main.log", " jump to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition (CX != 0) didn't meet: CX = 0x%04X\n", res_val);
            }
            break;
        }
        case 0xE3: {    // JCXZ SHORT-LABEL: [0xE3, IP-INC8]
            // JCXZ (Jump if CX Zero)
            ip_inc += 2;
            mylog("logs/main.log", "Instruction 0xE3: Relative Jump JCXZ");
            if (get_register_value(CX_register) == 0) {
                mylog("logs/main.log", " to 0x%02X\n", data[0]);
                ip_inc += ((int8_t*)data)[0];
            } else {
                mylog("logs/main.log", ": condition CX == 0 didn't meet: CX = 0x%04X\n", get_register_value(CX_register));
            }
            break;
        }
        case 0xE8: {  // CALL NEAR-PROC: [0xE8, IP-ING-LO, IP-INC-HI]
            int16_t offset = data[0] + (data[1] << 8);
            ip_inc += 3 + offset;
            push_register(get_register_value(IP_register)+3);
            mylog("logs/main.log", "Instruction 0xE8: Call: IP + 0x%04X\n", offset);
            break;
        }
        case 0xE9: {  // JMP NEAR-LABEL: [0xE9, IP-INC-LO, IP-INC-HI]
            int16_t offset = data[0] + (data[1] << 8);
            ip_inc += 3;
            ip_inc += offset;
            mylog("logs/main.log", "Instruction 0xE9: Relative jump to 0x%04X\n", offset);
            break;
        }
        case 0xEA: {    // JMP far label: [0xEA, IP-LO, IP-HI, CS-LO, CS-HI]
            set_register_value(IP_register, data[0] + (data[1] << 8));
            set_register_value(CS_register, data[2] + (data[3] << 8));
            ip_inc = 0;
            mylog("logs/main.log", "Instruction 0xEA: Far jump to IP = 0x%04X, CS = 0x%04X\n", REGS->IP, REGS->CS);
            break;
        }
        case 0xEB: {  // JMP SHORT-LABEL: [0xEB, IP-INC8]
            ip_inc += 2;
            ip_inc += ((int8_t*)data)[0];
            mylog("logs/main.log", "Instruction 0xEB: Relative jump to 0x%02X\n", data[0]);
            break;
        }
        default:
            printf("Error: Unknown JMP instruction: 0x%02X\n", opcode);
    }
    uint16_t new_ip = (int32_t)get_register_value(IP_register) + ip_inc;
    set_register_value(IP_register, new_ip);
    return 0;
}

uint8_t mov_instr(uint8_t opcode, uint8_t *data) {
    uint8_t ret_val = 1;
    switch(opcode) {
       case 0x88: {  // MOV REG8/MEM8, REG8: 0x88, MOD REG R/M, (DISP-LO), (DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0);
            mylog("logs/main.log", "Instruction 0x%02X: MOV %s (0x%04X), %s (0x%04X)\n", opcode, operands.destination, operands.dst_val, operands.source, operands.src_val);
            ret_val += operands.num_bytes;
            if (operands.dst_type == 0) {   // Register_mode
                set_register_value(operands.dst.register_name, operands.src_val);
                ret_val = 2;
            } else {    // Memory mode
                mem_write(operands.dst.address, operands.src_val, operands.width);
                ret_val = 4;
            }
            break;
        }
        case 0x89: {  // MOV REG16 REG16/MEM16: [0x89, MOD REG R/M, (DISP-LO),(DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0);
            mylog("logs/main.log", "Instruction 0x%02X: MOV %s (0x%04X), %s (0x%04X)\n", opcode, operands.destination, operands.dst_val, operands.source, operands.src_val);
            ret_val += operands.num_bytes;
            if (operands.dst_type == 0) {   // Register_mode
                set_register_value(operands.dst.register_name, operands.src_val);
            } else {    // Memory mode
                mem_write(operands.dst.address, operands.src_val, operands.width);
            }
            break;
        }
        case 0x8A: {  // MOV REG8, REG8/MEM8: [0x8A, MOD REG R/M, (DISP-LO),(DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0);
            mylog("logs/main.log", "Instruction 0x%02X: MOV %s, %s\n", opcode, operands.destination, operands.source);
            ret_val += operands.num_bytes;
            if (operands.dst_type == 0) {   // Register_mode
                set_register_value(operands.dst.register_name, operands.src_val);
            } else {    // Memory mode
                mem_write(operands.dst.address, operands.src_val, operands.width);
            }
            break;
        }
        case 0x8B: {    // MOV REG16, REG16/MEM16: [0x8B, MOD REG R/M, (DISP-LO), (DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0);
            mylog("logs/main.log", "Instruction 0x%02X: MOV %s, %s\n", opcode, operands.destination, operands.source);
            ret_val += operands.num_bytes;
            if (operands.dst_type == 0) {   // Register_mode
                set_register_value(operands.dst.register_name, operands.src_val);
            } else {    // Memory mode
                mem_write(operands.dst.address, operands.src_val, operands.width);
            }
            break;
        }
        case 0x8C: {    // MOV REG16/MEM16, SEGREG: [0x8C, MOD OSR R/M, (DISP-LO),(DISP-HI)]
            // reg_field: Segment register code: OO=ES, 01=CS, 10=SS, 11 =DS
            uint8_t reg_field = get_register_field(data[0]);
            operands_t operands = decode_operands(opcode | 0x01, data, 0); // Set width to 16-bit
            ret_val += operands.num_bytes;
            operands.src_type = 0;  // Force set operands src type
            if (reg_field == 0) {   // source = ES
                operands.src.register_name = ES_register;
            } else if (reg_field == 1) {
                operands.src.register_name = CS_register;
            } else if (reg_field == 2) {
                operands.src.register_name = SS_register;
            } else if (reg_field == 3) {
                operands.src.register_name = DS_register;
            } else {
                printf("ERROR: Incorrect reg field: opcode = 0x%02X, reg_field = 0x%02X\n", opcode, reg_field);
            }
            mylog("logs/main.log", "Instruction 0x%02X: MOV %s, %s\n", opcode, operands.destination, get_reg_name_string(operands.src.register_name));
            if (operands.dst_type == 0) {
                set_register_value(operands.dst.register_name, get_register_value(operands.src.register_name));
            } else {
                uint16_t value = get_register_value(operands.src.register_name);
                mem_write(operands.dst.address, value, 2);
            }
            break;
        }
        case 0x8E: {    // MOV SEGREG, REG16/MEM16: [0x8C, MOD OSR R/M, (DISP-LO),(DISP-HI) ]
            // reg_field: Segment register code: OO=ES, 01=CS, 10=SS, 11 =DS
            uint8_t reg_field = get_register_field(data[0]);
            operands_t operands = decode_operands(opcode | 0x01, data, 0); // Set width to 16-bit
            ret_val += operands.num_bytes;
            operands.dst_type = 0;  // Force set operands src type
            if (reg_field == 0) {   // source = ES
                operands.dst.register_name = ES_register;
            } else if (reg_field == 1) {
                operands.dst.register_name = CS_register;
            } else if (reg_field == 2) {
                operands.dst.register_name = SS_register;
            } else if (reg_field == 3) {
                operands.dst.register_name = DS_register;
            } else {
                printf("ERROR: Incorrect reg field: opcode = 0x%02X, reg_field = 0x%02X\n", opcode, reg_field);
            }
            mylog("logs/main.log", "Instruction 0x%02X: MOV %s, %s\n", opcode, operands.destination, operands.source);
            if (operands.src_type == 0) {
                set_register_value(operands.dst.register_name, get_register_value(operands.src.register_name));
            } else {
                set_register_value(operands.dst.register_name, mem_read(operands.src.address, 2));
            }
            break;
        }
        case 0xA0: {  // MOV AL, MEM8: [0xA0, DISP-LO, DISP-HI]
            uint16_t addr = get_addr(DS_register, data[0] + (data[1] << 8));
            uint16_t value = mem_read(addr, 1);
            mylog("logs/main.log", "Instruction 0x%02X: MOV AL, MEM8 (0x%04X @0x%08X)\n", opcode, value, addr);
            set_register_value(AL_register, value);
            ret_val = 3;
            break;
        }
        case 0xA1: {  // MOV AX, MEM16: [0xA1, DISP-LO, DISP-HI]
            uint16_t addr = get_addr(DS_register, data[0] + (data[1] << 8));
            uint16_t value = mem_read(addr, 2);
            mylog("logs/main.log", "Instruction 0x%02X: MOV AX, MEM16 (0x%04X @0x%08X)\n", opcode, value, addr);
            set_register_value(AX_register, value);
            break;
        }
        case 0xA2: {  // MOV MEM8, AL: [0xA2, DISP-LO, DISP-HI]
            uint16_t addr = get_addr(DS_register, data[0] + (data[1] << 8));
            mem_write(addr, get_register_value(AL_register), 1);
            ret_val = 3;
            mylog("logs/main.log", "Instruction 0x%02X: MOV MEM8 (@0x%08X), AL (0x%04X)\n", opcode, addr, get_register_value(AL_register));
            break;
        }
        case 0xA3: {  // MOV MEM16, AX: [0xA3, ADDR-LO, ADDR-HI]
            uint16_t addr = get_addr(DS_register, data[0] + (data[1] << 8));
            mem_write(addr, get_register_value(AX_register), 2);
            ret_val = 3;
            mylog("logs/main.log", "Instruction 0x%02X: MOV MEM16 (@0x%08X), AX (0x%04X)\n", opcode, addr, get_register_value(AX_register));
            break;
        }
        case 0xB0: {  // MOV AL, IMMED8: [0xB0, immed8]
            mylog("logs/main.log", "Instruction 0x%02X: MOV AL immed8 = 0x%02X\n", opcode, data[0]);
            set_register_value(AL_register, data[0]);
            ret_val = 2;
            break;
        }
        case 0xB1: {  // MOV CL, IMMED8: [0xB1, immed8]
            mylog("logs/main.log", "Instruction 0x%02X: MOV CL immed8 = 0x%02X\n", opcode, data[0]);
            set_register_value(CL_register, data[0]);
            ret_val = 2;
            break;
        }
        case 0xB2: {  // MOV DL, IMMED8: [0xB2, immed8]
            mylog("logs/main.log", "Instruction 0x%02X: MOV DL immed8 = 0x%02X\n", opcode, data[0]);
            set_register_value(DL_register, data[0]);
            ret_val = 2;
            break;
        }
        case 0xB3: {  // MOV BL, IMMED8: [0xB3, immed8]
            mylog("logs/main.log", "Instruction 0x%02X: MOV BL immed8 = 0x%02X\n", opcode, data[0]);
            set_register_value(BL_register, data[0]);
            ret_val = 2;
            break;
        }
        case 0xB4: {  // MOV AH, IMMED8: [0xB4, immed8]
            mylog("logs/main.log", "Instruction 0x%02X: MOV AH immed8 = 0x%02X\n", opcode, data[0]);
            set_register_value(AH_register, data[0]);
            ret_val = 2;
            break;
        }
        case 0xB5: {  // MOV CH, IMMED8: [0xB5, immed8]
            mylog("logs/main.log", "Instruction 0x%02X: MOV CH immed8 = 0x%02X\n", opcode, data[0]);
            set_register_value(CH_register, data[0]);
            ret_val = 2;
            break;
        }
        case 0xB6: {  // MOV DH, IMMED8: [0xB6, immed8]
            mylog("logs/main.log", "Instruction 0x%02X: MOV DH immed8 = 0x%02X\n", opcode, data[0]);
            set_register_value(DH_register, data[0]);
            ret_val = 2;
            break;
        }
        case 0xB7: {  // MOV BH, IMMED8: [0xB7, immed8]
            mylog("logs/main.log", "Instruction 0x%02X: MOV BH immed8 = 0x%02X\n", opcode, data[0]);
            set_register_value(BH_register, data[0]);
            ret_val = 2;
            break;
        }
        case 0xB8: {  // MOV AX, IMMED16: [0xB8, DATA-LO, DATA-HI]
            uint16_t immed_data = data[0] + (data[1] << 8);
            mylog("logs/main.log", "Instruction 0x%02X: MOV AX, IMMED16 = 0x%04X\n", opcode, immed_data);;
            set_register_value(AX_register, immed_data);
            ret_val = 3;
            break;
        }
        case 0xB9: {  // MOV CX, IMMED16: [0xB9, DATA-LO, DATA-HI]
            uint16_t immed_data = data[0] + (data[1] << 8);
            mylog("logs/main.log", "Instruction 0x%02X: MOV CX, IMMED16 = 0x%04X\n", opcode, immed_data);;
            set_register_value(CX_register, immed_data);
            ret_val = 3;
            break;
        }
        case 0xBA: {  // MOV DX, IMMED16: [0xBB, DATA-LO, DATA-HI]
            uint16_t immed_data = data[0] + (data[1] << 8);
            mylog("logs/main.log", "Instruction 0x%02X: MOV DX, IMMED16 = 0x%04X\n", opcode, immed_data);;
            set_register_value(DX_register, immed_data);
            ret_val = 3;
            break;
        }
        case 0xBB: {  // MOV BX, IMMED16: [0xBB, DATA-LO, DATA-HI]
            uint16_t immed_data = data[0] + (data[1] << 8);
            mylog("logs/main.log", "Instruction 0x%02X: MOV BX, IMMED16 = 0x%04X\n", opcode, immed_data);;
            set_register_value(BX_register, immed_data);
            ret_val = 3;
            break;
        }
        case 0xBC: {  // MOV SP, IMMED16: [0xBC, DATA-LO, DATA-HI]
            uint16_t immed_data = data[0] + (data[1] << 8);
            mylog("logs/main.log", "Instruction 0x%02X: MOV SP, IMMED16 = 0x%04X\n", opcode, immed_data);;
            set_register_value(SP_register, immed_data);
            ret_val = 3;
            break;
        }
        case 0xBD: {  // MOV BP, IMMED16: [0xBD, DATA-LO, DATA-HI]
            uint16_t immed_data = data[0] + (data[1] << 8);
            mylog("logs/main.log", "Instruction 0x%02X: MOV BP, IMMED16 = 0x%04X\n", opcode, immed_data);;
            set_register_value(BP_register, immed_data);
            ret_val = 3;
            break;
        }
        case 0xBE: {  // MOV SI, IMMED16: [0xBC, DATA-LO, DATA-HI]
            uint16_t immed_data = data[0] + (data[1] << 8);
            mylog("logs/main.log", "Instruction 0x%02X: MOV SI, IMMED16 = 0x%04X\n", opcode, immed_data);;
            set_register_value(SI_register, immed_data);
            ret_val = 3;
            break;
        }
        case 0xBF: {  // MOV DI, IMMED16: [0xBC, DATA-LO, DATA-HI]
            uint16_t immed_data = data[0] + (data[1] << 8);
            mylog("logs/main.log", "Instruction 0x%02X: MOV DI, IMMED16 = 0x%04X\n", opcode, immed_data);
            set_register_value(DI_register, immed_data);
            ret_val = 3;
            break;
        }
        case 0xC6: {  // MOV MEM8, IMMED8: [0xC6, MOD 000 R/M, DISP-LO, DISP-HI, DATA-8]
            if(get_register_field(data[0]) == 0) {
                uint16_t addr = get_addr(DS_register, data[1] + (data[2] << 8));
                mem_write(addr, data[3], 1);
                mylog("logs/main.log", "Instruction 0x%02X: MOV MEM8 (@0x%08X), IMMED8 (0x%02X)\n", opcode, addr, data[3]);
                ret_val = 5;
            } else {
                REGS->invalid_operations ++;
                printf("Invalid instruction: 0x%02X, REG field != 0\n", opcode);
            }
            break;
        }
        case 0xC7: {  // MOV MEM16, IMMED16: [0xC7, MOD 000 R/M, (DISP-LO), (DISP-HI), DATA-LO, DATA-HI]
            if(get_register_field(data[0]) == 0) {
                operands_t operands = decode_operands(opcode, data, 1);
                ret_val += operands.num_bytes + 2;
                uint16_t value = data[operands.num_bytes] + (data[operands.num_bytes+1] << 8);
                mem_write(operands.dst.address, value, 2);
                mylog("logs/main.log", "Instruction 0x%02X: MOV MEM16 (@0x%08X), IMMED16 (0x%04X)\n", opcode, operands.dst.address, value);
            } else {
                REGS->invalid_operations ++;
                printf("Invalid instruction: 0x%02X, REG field != 0\n", opcode);
            }
            break;
        }
        default:
            printf("Error: Unknown MOV instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t shift_instr(uint8_t opcode, uint8_t *data) {
    uint8_t ret_val = 1;
    uint16_t res_val = 0;
    switch(opcode) {
        case 0xD0:      // 8-bit shifts by 1
        case 0xD1:      // 16-bit shifts by 1
        case 0xD2:      // 8-bit shifts by CL
        case 0xD3: {    // 16-bit shifts by CL
            operands_t operands = decode_operands(opcode, data, 1);
            ret_val += operands.num_bytes;
            if((opcode == 0xD0) || (opcode == 0xD1)) {
                operands.src_val = 1;
            } else if((opcode == 0xD2) || (opcode == 0xD3)) {
                operands.src_val = get_register_value(CL_register);
            }
            switch(get_register_field(data[0])) {
                case 0:   // ROL REG/MEM, 1/CL: [opcode, MOD 000 R/M, DISP-LO, DISP-HI]
                    // rotr32(x, n) (( x>>n  ) | (x<<(64-n)))
                    if(operands.src_val > 16) {
                        operands.src_val = operands.src_val % 16;
                    }
                    operands.dst_val = (operands.dst_val << operands.src_val) | (operands.dst_val >> (16 - operands.src_val));
                    update_flags(operands.dst_val, operands.src_val, res_val, operands.width, SHIFT_L_OP);
                    mylog("logs/main.log", "Instruction 0x%02X: ROL %s (0x%04X), %d: result = 0x%04X\n", opcode, operands.destination, operands.dst_val, operands.src_val, res_val);
                    break;
                case 1: // ROR REG/MEM, 1/CL: [opcode, MOD 001 R/M, DISP-LO, DISP-HI]
                    printf("ERROR: unimplemented ROR REG/MEM, 1/CL operation\n");
                    REGS->invalid_operations ++;
                    break;
                case 2: // RCL REG/MEM, 1/CL: [opcode, MOD 010 R/M, DISP-LO, DISP-HI]
                    printf("ERROR: unimplemented RCL REG/MEM, 1/CL operation\n");
                    REGS->invalid_operations ++;
                    break;
                case 3: // RCR REG/MEM, 1/CL: [opcode, MOD 011 R/M, DISP-LO, DISP-HI]
                    printf("ERROR: unimplemented RCR REG/MEM, 1/CL operation\n");
                    REGS->invalid_operations ++;
                    break;
                case 4: // SAL/SHL REG/MEM, 1/CL, 1: [opcode, MOD 100 R/M, DISP-LO, DISP-HI]
                    res_val = operands.dst_val << operands.src_val;
                    update_flags(operands.dst_val, operands.src_val, res_val, operands.width, SHIFT_L_OP);
                    mylog("logs/main.log", "Instruction 0x%02X: SAL/SHL %s (0x%04X), %d: dst , result = 0x%02X\n", opcode, operands.destination, operands.dst_val, operands.src_val, res_val);
                    break;
                case 5: // SHR REG/MEM, 1/CL: [opcode, MOD 101 R/M, DISP-LO, DISP-HI]
                    res_val = operands.dst_val >> operands.src_val;
                    update_flags(operands.dst_val, operands.src_val, res_val, 2, SHIFT_R_OP);
                    mylog("logs/main.log", "Instruction 0x%02X: SHR %s (0x%04X), CL %d; result = 0x%02X\n", opcode, operands.destination, operands.dst_val, operands.src_val, res_val);
                    break;
                case 6: // INVALID_INSTRUCTION
                    REGS->invalid_operations ++;
                    printf("Invalid instruction: 0x%02X, REG field 110\n", opcode);
                    break;
                case 7: // SAR REG/MEM, 1/CL: [opcode, MOD 111 R/M, DISP-LO, DISP-HI]
                    printf("ERROR: unimplemented SAR REG/MEM, 1/CL operation\n");
                    REGS->invalid_operations ++;
                    break;
            }
            if(operands.dst_type == 0) {    // Register mode
                set_register_value(operands.dst.register_name, res_val);
            } else {    // Memory mode
                mem_write(operands.dst.address, res_val, operands.width);
            }
            break;
        }
        default:
            printf("Error: Unknown Shift/Rotate instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t cmp_instr(uint8_t opcode, uint8_t *data) {
    // CMP updates AF, CF, OF, PF, SF and ZF
    uint8_t ret_val = 1;
    int16_t res_val = 0;
    operands_t operands = {0};
    switch(opcode) {
        case 0x38:  // CMP REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x39:  // CMP REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x3A:  // CMP REG8, REG8/MEM8      [0x3A, MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x3B: {// CMP REG16, REG16/MEM16   [0x3B, MOD REG R/M, (DISP-LO), (DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0);
            ret_val += operands.num_bytes;
            res_val = operands.dst_val - operands.src_val;
            update_flags(operands.dst_val, operands.src_val, res_val, operands.num_bytes, SUB_OP);
            mylog("logs/main.log", "Instruction 0x%02X: CMP %s (0x%04X), %s (0x%04X); result = 0x%04X\n", opcode, operands.destination, operands.dst_val, operands.source, operands.src_val, res_val);
            break;
        }
        case 0x3C: {  // CMP AL, IMMED8: [0x3C, DATA-8]
            operands.src_val = data[0];
            operands.dst_val = get_register_value(AL_register);
            res_val = operands.dst_val - operands.src_val;
            mylog("logs/main.log", "Instruction 0x3D: CMP AX immed16 = 0x%04X\n", operands.src_val);
            update_flags(operands.dst_val, operands.src_val, res_val, 2, SUB_OP);
            ret_val = 2;
            break;
        }
        case 0x3D: {  // CMP AX, immed16: [0x3D, DATA-LO, DATA-HI]
            operands.src_val = (data[1] << 8) + data[0];
            operands.dst_val = get_register_value(AX_register);
            res_val = operands.dst_val - operands.src_val;
            mylog("logs/main.log", "Instruction 0x3D: CMP AX immed16 = 0x%04X\n", operands.src_val);
            update_flags(operands.dst_val, operands.src_val, res_val, 2, SUB_OP);
            ret_val = 3;
            break;
        }
        default:
            printf("Error: Unknown CMP instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t div_instr(uint8_t opcode, uint8_t *data) {
    // If the source operand is a byte, it is divided into the double-length dividend assumed
    // to be in registers AL and AH. The single-length quotient is returned in AL, and the single-length
    // remainder is returned in AH. If the source operand is a word, it is divided into the double-
    // length dividend in registers AX and DX. The single-length quotient is returned in AX, and the
    // single-length remainder is returned in DX. If the quotient exceeds the capacity of its destination
    // register (FFH for byte source, FFFFFH for word source), as when division by zero is attempted, a
    // type 0 interrupt is generated, and the quotient and remainder are undefined. Nonintegral quotients
    // are truncated to integers. The content of AF, CF, OF, PF, SF and ZF is undefined following execution of DIV. (p 60)
    uint8_t ret_val = 1;
    switch(opcode) {
        case 0x00: {    // DIV REG8/MEM8: [0xFFFF, MOD 110 R/M, (DISP-LO),(DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0); // Treat as regular as the only operand is SRC
            ret_val += operands.num_bytes;
            operands.dst_val = get_register_value(AL_register);
            uint16_t res = operands.dst_val / operands.src_val;
            if(res > 0xFF) {
                set_int_vector(0);
            }
            set_register_value(AL_register, res & 0xFF);
            set_register_value(AH_register, (uint8_t)(operands.dst_val % operands.src_val));
            break;
        }
        case 0xF7: {    // DIV REG16/MEM16: [0xF7, MOD 110 R/M, (DISP-LO),(DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0); // Treat as regular as the only operand is SRC
            ret_val += operands.num_bytes;
            operands.dst_val = get_register_value(AX_register);
            uint32_t res = operands.dst_val / operands.src_val;
            if(res > 0xFFFF) {
                set_int_vector(0);
            }
            set_register_value(AX_register, res & 0xFFFF);
            set_register_value(DX_register, (uint16_t)(operands.dst_val % operands.src_val));
            break;
        }
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid MUL instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t mul_instr(uint8_t opcode, uint8_t *data) {
    uint8_t ret_val = 1;
    switch(opcode) {
        case 0xF6: {
            uint8_t reg_field = get_register_field(data[0]);
            if(reg_field == 5) { // IMUL REG8/MEM8 (signed): [0xF6, MOD 101 R/M, (DISP-LO), (DISP-HI)]
                operands_t operands = decode_operands(opcode, data, 0);
                ret_val += operands.num_bytes;
                int16_t res_val = (int16_t)get_register_value(AL_register) * operands.src_val;
                set_register_value(AX_register, res_val);
                if(get_register_value(AH_register) == 0) {
                    set_flag(CF, 0);
                    set_flag(OF, 0);
                } else if((get_register_value(AH_register) == 0xFF) && ((get_register_value(AL_register) & 0x80) > 0)) {
                    set_flag(CF, 0);
                    set_flag(OF, 0);
                } else {
                    set_flag(CF, 1);
                    set_flag(OF, 1);
                }
            }
            break;
        }
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid MUL instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t add_instr(uint8_t opcode, uint8_t *data) {
    uint8_t ret_val = 1;
    int16_t res_val = 0;
    switch(opcode) {
        case 0x81: {  // ADD REG16/MEM16, IMMED16: [0x81, MOD 000 R/M, (DISP-LO), (DISP-HI), DATA-LO, DATA-HI]
            operands_t operands = decode_operands(opcode, data, 1);
            ret_val += operands.num_bytes + 2;
            uint8_t reg_field = get_register_field(data[0]);
            if(reg_field != 0) {
                REGS->invalid_operations ++;
                printf("Error: Invalid ADD instruction: 0x%02X, reg_field = %d\n", opcode, reg_field);
            }
            res_val = operands.src_val + operands.dst_val;
            mylog("logs/main.log", "Instruction 0x%02X: ADD %s (0x%04X), immed16 (0x%04X); res = 0x%04X\n", opcode, operands.destination, operands.dst_val, operands.src_val, res_val);
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, ADD_OP);
            if(operands.dst_type == 0) {    // Register mode
                set_register_value(operands.dst.register_name, res_val);
            } else {    // Memory mode
                mem_write(operands.dst.address, res_val, operands.width);
            }
            break;
        }
        case 0x83: {    // ADD REG16/MEM16, IMMED8: [0x83, MOD 000 R/M, (DISP-LO),(DISP-HI), DATA-SX]
            operands_t operands = decode_operands(opcode, data, 1);
            ret_val += operands.num_bytes + 1;
            operands.src_val = (int16_t)(data[operands.num_bytes]);
            uint8_t reg_field = get_register_field(data[0]);
            if(reg_field != 0) {
                REGS->invalid_operations ++;
                printf("Error: Invalid ADD instruction: 0x%02X, reg_field = %d\n", opcode, reg_field);
            }
            res_val = operands.src_val + operands.dst_val;
            mylog("logs/main.log", "Instruction 0x%02X: ADD %s (0x%04X), (int16_t)immed8 (0x%04X); res = 0x%04X\n", opcode, operands.destination, operands.dst_val, operands.src_val, res_val);
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, ADD_OP);
            if(operands.dst_type == 0) {    // Register mode
                set_register_value(operands.dst.register_name, res_val);
            } else {    // Memory mode
                mem_write(operands.dst.address, res_val, operands.width);
            }
            break;
        }
        case 0x00:      // ADD REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x01:      // ADD REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x02:      // ADD REG8, REG8/MEM8: [0x02, MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x03: {    // ADD REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0);
            ret_val += operands.num_bytes;
            res_val = operands.dst_val + operands.src_val;
            mylog("logs/main.log", "Instruction 0x%02X: ADD %s (0x%04X), %s (0x%04X); res = 0x%04X\n", opcode, operands.destination, operands.dst_val, operands.source, operands.src_val, res_val);
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, ADD_OP);
            if(operands.dst_type == 0) {    // Register mode
                set_register_value(operands.dst.register_name, res_val);
            } else {    // Memory mode
                mem_write(operands.dst.address, res_val, operands.width);
            }
            break;
        }
        case 0x04: {  // ADD AL, IMMED8, [0x04, DATA-8]
            int8_t src_val = data[0];
            int8_t dst_val = get_register_value(AL_register);
            res_val = dst_val + src_val;
            mylog("logs/main.log", "Instruction 0x%02X: ADD AL (0x%04X), immed (0x%04X); res = 0x%04X\n", opcode, dst_val, src_val, res_val);
            update_flags(dst_val, src_val, res_val, 1, ADD_OP);
            set_register_value(AL_register, res_val);
            ret_val = 2;
            break;
        }
        case 0x05: {    // ADD AX, IMMED16: [0x05, DATA-LO, DATA-HI]
            int16_t src_val = (data[1] << 8) + data[0];
            int16_t dst_val = get_register_value(AX_register);
            res_val = dst_val + src_val;
            mylog("logs/main.log", "Instruction 0x%02X: ADD AX (0x%04X), immed (0x%04X); res = 0x%04X\n", opcode, dst_val, src_val, res_val);
            update_flags(dst_val, src_val, res_val, 2, ADD_OP);
            set_register_value(AX_register, res_val);
            ret_val = 3;
            break;
        }
        case 0x10:  // ADC REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x11:  // ADC REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x12: {  // ADC REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0);
            ret_val += operands.num_bytes;
            res_val = operands.dst_val + operands.src_val;
            if(get_flag(CF)) {
                res_val += 1;
            }
            mylog("logs/main.log", "Instruction 0x%02X: ADC %s (0x%02X), %s (0x%02X), CF = %d; res = 0x%02X\n", opcode, operands.destination, operands.dst_val, operands.source, operands.src_val, get_flag(CF), res_val);
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, ADD_OP);
            if(operands.dst_type == 0) {    // Register mode
                set_register_value(operands.dst.register_name, res_val);
            } else {                        // Memory mode
                mem_write(operands.dst.address, res_val, operands.width);
            }
            break;
        }
        case 0x13: {  // ADC REG16, REG16/MEM16:   [MOD REG R/M, (DISP-LO), (DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0);
            ret_val += operands.num_bytes;
            res_val = operands.dst_val + operands.src_val;
            if(get_flag(CF)) {
                res_val += 1;
            }
            mylog("logs/main.log", "Instruction 0x%02X: ADC %s (0x%04X), %s (0x%04X), CF = %d; res = 0x%04X\n", opcode, operands.destination, operands.dst_val, operands.source, operands.src_val, get_flag(CF), res_val);
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, ADD_OP);
            if(operands.dst_type == 0) {    // Register mode
                set_register_value(operands.dst.register_name, res_val);
            } else {                        // Memory mode
                mem_write(operands.dst.address, res_val, operands.width);
            }
            break;
        }
        case 0x14: {  // ADC AL, IMMED8: [DATA-8]
            operands_t operands = {0};
            operands.dst_val = get_register_value(AL_register);
            operands.src_val = data[0];
            res_val = operands.src_val + operands.dst_val;
            if(get_flag(CF)) {
                res_val += 1;
            }
            mylog("logs/main.log", "Instruction 0x%02X: ADC AL (0x%04X), immed8 (0x%04X), CF = %d; res = 0x%04X\n", opcode, operands.dst_val, operands.src_val, get_flag(CF), res_val);
            update_flags(operands.dst_val, operands.src_val, res_val, 1, ADD_OP);
            set_register_value(AL_register, res_val);
            ret_val = 2;
            break;
        }
        case 0x15: {  // ADC AX, immed16: [DATA-LO, DATA-HI]
            operands_t operands = {0};
            operands.dst_val = get_register_value(AL_register);
            operands.src_val = data[0] + (data[1] << 8);
            res_val = operands.src_val + operands.dst_val;
            if(get_flag(CF)) {
                res_val += 1;
            }
            mylog("logs/main.log", "Instruction 0x%02X: ADC AL (0x%04X), immed16 (0x%04X), CF = %d; res = 0x%04X\n", opcode, operands.dst_val, operands.src_val, get_flag(CF), res_val);
            update_flags(operands.dst_val, operands.src_val, res_val, 2, ADD_OP);
            set_register_value(AL_register, res_val);
            ret_val = 3;
            break;
        }
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid ADD instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t sub_instr(uint8_t opcode, uint8_t *data) {
    uint8_t ret_val = 1;
    int16_t res_val = 0;
    switch(opcode) {
        case 0x80: {    // INSTR REG8/MEM8, IMMED8: [0x80, MOD 111 R/M, (DISP-LO), (DISP-HI), DATA-8]
            operands_t operands = decode_operands(opcode, data, 1);
            ret_val += operands.num_bytes + operands.width;
            res_val = operands.dst_val - operands.src_val;
            uint8_t reg_field = get_register_field(data[0]);
            if(reg_field == 3) {  // SBB REG8/MEM8, IMMED8: [0x80, MOD 011 R/M, (DISP-LO), (DISP-HI), DATA-8]
                if(get_flag(CF)) {
                    res_val -= 1;
                }
                mylog("logs/main.log", "Instruction 0x80[3] SBB %s (0x%02X), immed8 (0x%02X), CF = %d; res = 0x%02X\n", operands.destination, operands.dst_val, operands.src_val, get_flag(CF), res_val);
                if(operands.dst_type == 0) {    // Register mode
                    set_register_value(operands.dst.register_name, res_val);
                } else {    // Memory mode
                    mem_write(operands.dst.address, res_val, operands.width);
                }
            } else if(reg_field == 7) { // CMP REG8/MEM8, IMMED8: [0x80, MOD 111 R/M, (DISP-LO), (DISP-HI), DATA-8]
                mylog("logs/main.log", "Instruction 0x80[7]: CMP %s (0x%02X), immed8 (0x%02X); res = 0x%02X\n", operands.destination, operands.dst_val, operands.src_val, res_val);
            } else {
                REGS->invalid_operations ++;
                printf("Error: Invalid SUB instruction: 0x%02X, reg_field = %d\n", opcode, reg_field);
            }
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, SUB_OP);
            break;
        }
        case 0x81: { // INSTR REG16/MEM16, IMMED16: [0x81, MOD 111 R/M, (DISP-LO), (DISP-HI), DATA-LO, DATA-HI]
            operands_t operands = decode_operands(opcode, data, 1);
            ret_val += operands.num_bytes + operands.width;
            res_val = operands.dst_val - operands.src_val;
            uint8_t reg_field = get_register_field(data[0]);
            char *operation;
            if(reg_field == 3) {  // SBB REG16/MEM16,IMMED16: [0x81, MOD 111 R/M, (DISP-LO), (DISP-HI), DATA-LO, DATA-HI]
                operation = "SBB";
                if(get_flag(CF)) {
                    res_val -= 1;
                }
            } else if(reg_field == 5) { // SUB REG16/MEM16, IMMED8: [0x83, MOD 101 RIM, (DISP-LO),(DISP-HI), DATA-SX]
                operation = "SUB";
            } else if(reg_field == 7) { // CMP REG16/MEM16,IMMED16: [0x81, MOD 111 R/M, (DISP-LO), (DISP-HI), DATA-LO, DATA-HI]
                operation = "CMP";
            }
            mylog("logs/main.log", "Instruction 0x%02X[%d] %s %s (0x%04X), immed16 (0x%04X), CF = %d; res = 0x%02X\n", opcode, reg_field, operation, operands.destination, operands.dst_val, operands.src_val, get_flag(CF), res_val);
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, SUB_OP);
            if(reg_field != 7) {
                if(operands.dst_type == 0) {    // Register mode
                    set_register_value(operands.dst.register_name, res_val);
                } else {    // Register mode
                    mem_write(operands.dst.address, res_val, operands.width);
                }
            }
            break;
        }
        case 0x83: {
            // DATA-SX: 8-bit immediate value that is automatically sign-extended to 16-bits before use.
            uint8_t reg_field = get_register_field(data[0]);
            operands_t operands = decode_operands(opcode, data, 1);
            ret_val += operands.num_bytes + 1;
            operands.src_val = (int16_t)(data[operands.num_bytes]);
            char *operation;
            res_val = operands.dst_val - operands.src_val;
            if(reg_field == 3) {        // SBB REG16/MEM16, IMMED8: [0x83, MOD 011 R/M, (DISP-LO), (DISP-HI), DATA-SX]
                if(get_flag(CF) > 0) {
                    res_val -= 1;
                }
                operation = "SBB";
            } else if(reg_field == 5) { // SUB REG16/MEM16, IMMED8: [0x83, MOD 101 RIM, (DISP-LO),(DISP-HI), DATA-SX]
                operation = "SUB";
            } else if(reg_field == 7) { // CMP REG16/MEM16, IMMED8: [0x83, MOD 101 RIM, (DISP-LO),(DISP-HI), DATA-SX]
                operation = "CMP";
            } else {
                REGS->invalid_operations ++;
                printf("Error: Invalid SUB instruction: 0x%02X, reg_field = %d\n", opcode, reg_field);
                return res_val;
            }
            mylog("logs/main.log", "Instruction 0x%02X: %s %s (0x%04X), immed8 (DATA-SX) (0x%04X), res = 0x%04X\n", opcode, operation, operands.destination, operands.dst_val, operands.src_val, res_val);
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, SUB_OP);
            if(reg_field != 7) {
                if(operands.dst_type == 0) {    // Register mode
                    set_register_value(operands.dst.register_name, res_val);
                } else {    // Memory mode
                    mem_write(operands.dst.address, res_val, operands.width);
                }
            }
            break;
        }
        case 0x18:  // SBB REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x19:  // SBB REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x1A:  // SBB REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x1B:  // SBB REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x28:  // SUB REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x29:  // SUB REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x2A:  // SUB REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x2B: {// SUB REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0);
            ret_val += operands.num_bytes;
            res_val = operands.dst_val - operands.src_val;
            mylog("logs/main.log", "Instruction 0x%02X: SUB %s (0x%04X), %s (0x%04X), res = 0x%04X\n", opcode, operands.destination, operands.dst_val, operands.destination, operands.src_val, res_val);
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, SUB_OP);
            if(operands.dst_type == 0) {
                set_register_value(operands.dst.register_name, res_val);
            } else {    // Memory mode
                mem_write(operands.dst.address, res_val, operands.width);
            }
            break;
        }
        case 0x1C:  // SBB AL, IMMED8   [DATA-8]
        case 0x1D:  // SBB AX, immed16  [DATA-LO, DATA-HI]
        case 0x2C:  // SUB AL, IMMED8:  [DATA-8]
        case 0x2D: {// SUB AX, immed16: [DATA-LO, DATA-HI]
            operands_t operands = {0};
            if((opcode == 0x2C) || (opcode == 0x1C)) {
                operands.src_val = get_register_value(AL_register);
                operands.dst_val = data[0];
                operands.width = 1;
                ret_val = 2;
            } else if((opcode == 0x2D) || (opcode == 0x1D)) {
                operands.src_val = get_register_value(AX_register);
                operands.dst_val = data[0] + (data[1] << 8);
                operands.width = 2;
                ret_val = 3;
            }
            res_val = operands.dst_val - operands.src_val;
            if((opcode == 0x1C) || (opcode == 0x1D)) {
                if(get_flag(CF)) {
                    res_val -= 1;
                }
            }
            mylog("logs/main.log", "Instruction 0x%02X: SUB %s (0x%04X), %s (0x%04X), res = 0x%04X\n", opcode, operands.destination, operands.dst_val, operands.destination, operands.src_val, res_val);
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, SUB_OP);
            if(opcode == 0x2C) {
                set_register_value(AL_register, res_val);
            } else if(opcode == 0x2D) {
                set_register_value(AX_register, res_val);
            }
            break;
        }
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid SUB instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t sahf_instr(void) {
    // Loads the SF, ZF, AF, PF, and CF flags of the EFLAGS register with
    // values from the corresponding bits in the AH register (7, 6, 4, 2, 0 respectively)
    uint8_t AH = get_h(REGS->AX);
    mylog("logs/main.log", "Instruction 0x9E: SAHF: AH = 0x%02X, flags before: 0x%04X, ", AH, REGS->flags);
    set_flag(CF, AH & 0x01);
    set_flag(PF, AH & 0x04);
    set_flag(AF, AH & 0x10);
    set_flag(ZF, AH & 0x40);
    set_flag(SF, AH & 0x80);
    mylog("logs/main.log", "flags after: 0x%04X\n", REGS->flags);
    return 1;
}

uint8_t lahf_instr(void) {
    // Loads lower byte from the flags register into AH register
    mylog("logs/main.log", "Instruction 0x9F: LAHF: flags = 0x%04X, AX before: 0x%04X, ", REGS->flags, REGS->AX);
    set_register_value(AH_register, 0xFF & REGS->flags);
    mylog("logs/main.log", "AX after: 0x%04X\n", REGS->AX);
    return 1;
}

uint8_t xor_instr(uint8_t opcode, uint8_t *data) {
    uint8_t ret_val = 1;
    uint16_t res_val = 0;
    switch(opcode) {
        case 0x30:      // XOR REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x31:      // XOR REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x32:      // XOR REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x33: {    // XOR REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0);
            ret_val += operands.num_bytes;
            res_val = operands.src_val ^ operands.dst_val;
            if(operands.dst_type == 0) {    // Register mode
                set_register_value(operands.dst.register_name, res_val);
            } else {    // Memory mode
                mem_write(operands.dst.address, res_val, operands.width);
            }
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, LOGIC_OP);
            mylog("logs/main.log", "Instruction 0x%02X: XOR %s, %s: dst = 0x%04X, src = 0x%04X, result = 0x%04X\n", opcode, operands.destination, operands.source, operands.dst_val, operands.src_val, res_val);
            break;
        }
        case 0x34:  // XOR AL, IMMED8: [0x34, DATA-8]
            REGS->invalid_operations ++;
            printf("Error: Invalid XOR instruction: 0x%02X\n", opcode);
            break;
        case 0x35:  // XOR AX, immed16: [0x35, DATA-LO, DATA-HI]
            REGS->invalid_operations ++;
            printf("Error: Invalid XOR instruction: 0x%02X\n", opcode);
            break;
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid XOR instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t or_instr(uint8_t opcode, uint8_t *data) {
    uint8_t ret_val = 1;
    uint16_t res_val = 0;
    switch(opcode) {
        case 0x08:  // OR REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
            REGS->invalid_operations ++;
            printf("Error: Invalid OR instruction: 0x%02X\n", opcode);
            break;
        case 0x09:  // OR REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
            REGS->invalid_operations ++;
            printf("Error: Invalid OR instruction: 0x%02X\n", opcode);
            break;
        case 0x0A: { // OR REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0);
            ret_val += operands.num_bytes;
            res_val = operands.src_val | operands.dst_val;
            if(operands.dst_type == 0) {    // Register mode
                set_register_value(operands.dst.register_name, res_val);
            } else {    // Memory mode
                mem_write(operands.dst.address, res_val, 2);
            }
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, LOGIC_OP);
            mylog("logs/main.log", "Instruction 0x0A: OR %s (0x%02X), %s (0x%02X); result = 0x%02X\n", operands.destination, operands.dst_val, operands.source, operands.src_val, res_val);
            break;
        }
        case 0x0B: {  // OR REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0);
            ret_val += operands.num_bytes;
            res_val = operands.src_val | operands.dst_val;
            if(operands.dst_type == 0) {    // Register mode
                set_register_value(operands.dst.register_name, res_val);
            } else {    // Memory mode
                mem_write(operands.dst.address, res_val, 2);
            }
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, LOGIC_OP);
            mylog("logs/main.log", "Instruction 0x%02X: OR %s (0x%04X), %s (0x%04X); result = 0x%04X\n", opcode, operands.destination, operands.dst_val, operands.source, operands.src_val, res_val);
            break;
        }
        case 0x0C: {  // OR AL, IMMED8 [DATA-8]
            operands_t operands = {0};
            operands.src_val = data[0];
            operands.dst_val = get_register_value(AL_register);
            uint8_t res_val = operands.dst_val | operands.src_val;
            set_register_value(AL_register, res_val);
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, LOGIC_OP);
            mylog("logs/main.log", "Instruction 0x%02X: OR %s (0x%04X), %s (0x%04X); result = 0x%04X\n", opcode, operands.destination, operands.dst_val, operands.source, operands.src_val, res_val);
            ret_val += 1;
            break;
        }
        case 0x0D:  // OR AX, immed16 [DATA-LO, DATA-HI]
            REGS->invalid_operations ++;
            printf("Error: Invalid OR instruction: 0x%02X\n", opcode);
            break;
        case 0x80: {  // OR REG8/MEM8, IMMED8: [0x80, MOD 001 R/M, (DISP-LO), (DISP-HI), DATA-8]
            operands_t operands = decode_operands(opcode, data, 1);
            ret_val += operands.num_bytes + operands.width;
            if(operands.dst_type == 0) {    // Register mode
                set_register_value(operands.dst.register_name, res_val);
            } else {    // Memory mode
                mem_write(operands.dst.address, res_val, 1);
            }
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, LOGIC_OP);
            mylog("logs/main.log", "Instruction 0x%02X: OR %s (0x%04X), immed8 (0x%04X); result = 0x%04X\n", opcode, operands.destination, operands.dst_val, operands.src_val, res_val);
            break;
        }
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid OR instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t adj_instr(uint8_t opcode, uint8_t *data) {
    uint8_t ret_val = 1;
    switch(opcode) {
        case 0x27: {  // DAA
            // https://www.righto.com/2023/01/understanding-x86s-decimal-adjust-after.html
            uint8_t src_val = get_register_value(AL_register);
            uint8_t res_val = src_val;
            if(((res_val & 0x0F) > 9) || (get_flag(AF) > 0)) {
                res_val += 6;
            }
            if((src_val > 0x99) || (get_flag(CF) > 0)) {
                res_val += 0x60;
            }
            set_flag(SF, (res_val & 0x80) > 0);
            set_flag(ZF, (res_val & 0xFF) == 0);
            set_flag(PF, get_parity(res_val & 0xFF));
            mylog("logs/main.log", "Instruction 0x%02X: DAA AL = (0x%02X); result = 0x%04X\n", opcode, src_val, res_val);
            break;
        }
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid ADJUST instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}
uint8_t and_instr(uint8_t opcode, uint8_t *data) {
    uint8_t ret_val = 1;
    uint16_t res_val = 0;
    switch(opcode) {
        case 0x20:  // AND REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x21:  // AND REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x22:  // AND REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x23: {// AND REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0);
            ret_val += operands.num_bytes;
            res_val = operands.src_val & operands.dst_val;
            if(operands.dst_type == 0) {    // Register mode
                set_register_value(operands.dst.register_name, res_val);
            } else {    // Memory mode
                mem_write(operands.dst.address, res_val, 2);
            }
            update_flags(operands.dst_val, operands.src_val, res_val, operands.width, LOGIC_OP);
            mylog("logs/main.log", "Instruction 0x%02X: AND %s (0x%02X), %s (0x%02X); result = 0x%04X\n", opcode, operands.destination, operands.dst_val, operands.source, operands.src_val, res_val);
            break;
        }
        case 0x24: {  // AND AL, IMMED8: [0x24, DATA-8]
            operands_t operands = {0};
            operands.dst_val = get_register_value(AL_register);
            operands.src_val = data[0];
            res_val = operands.src_val & operands.dst_val;
            set_register_value(AL_register, res_val);
            update_flags(operands.dst_val, operands.src_val, res_val, 1, LOGIC_OP);
            mylog("logs/main.log", "Instruction 0x%02X: AND AL (0x%02X), immed8 (0x%02X); result = 0x%04X\n", opcode, operands.dst_val, operands.src_val, res_val);
            ret_val = 2;
            break;
        }
        case 0x25: {  // AND AX, immed16: [DATA-LO, DATA-HI]
            operands_t operands = {0};
            operands.dst_val = get_register_value(AL_register);
            operands.src_val = data[0] + (data[1] << 8);
            res_val = operands.src_val & operands.dst_val;
            set_register_value(AL_register, res_val);
            update_flags(operands.dst_val, operands.src_val, res_val, 2, LOGIC_OP);
            mylog("logs/main.log", "Instruction 0x%02X: AND AL (0x%02X), immed8 (0x%02X); result = 0x%04X\n", opcode, operands.dst_val, operands.src_val, res_val);
            ret_val = 3;
            break;
        }
        case 0xF6: {    // TEST REG8/MEM8, IMMED8: [0xF6, MOD 000 R/M, (DISP-LO), (DISP-HI), DATA-8]
            uint8_t reg_field = get_register_field(data[0]);
            if(0 != reg_field) {
                REGS->invalid_operations ++;
                printf("Error: Invalid TEST instruction: 0x%02X: reg_field = %d\n", opcode, reg_field);
                return ret_val;
            }
            operands_t operands = decode_operands(opcode, data, 1);
            ret_val += operands.num_bytes + operands.width;
            res_val = operands.dst_val & operands.src_val;
            update_flags(operands.dst_val, operands.src_val, res_val, 1, LOGIC_OP);
            if(operands.dst_type == 0) {    // Register_mode
                mylog("logs/main.log", "Instruction 0x%02X: TEST %s (0x%02X), immed8 (0x%02X); result = 0x%04X\n", opcode, operands.destination, operands.dst_val, operands.src_val, res_val);
            } else {
                mylog("logs/main.log", "Instruction 0x%02X: TEST %s (0x%02X @ 0x%06X), immed8 (0x%02X); result = 0x%04X\n", opcode, operands.destination, operands.dst_val, operands.dst.address, operands.src_val, res_val);
            }
            break;
        }
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid AND instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t pop_reg_instr(uint8_t opcode, uint8_t *data) {
    uint8_t ret_val = 1;
    register_name_t reg;
    switch(opcode) {
        case 0x07:  // POP ES
        case 0x17:  // POP SS
        case 0x1F:  // POP DS
        case 0x58:  // POP AX
        case 0x59:  // POP CX
        case 0x5A:  // POP DX
        case 0x5B:  // POP BX
        case 0x5C:  // POP SP
        case 0x5D:  // POP BP
        case 0x5E:  // POP SI
        case 0x5F:  // POP DI
        case 0x9D: {// POPF
            if(opcode == 0x07) {
                reg = ES_register;
            } else if(opcode == 0x17) {
                reg = SS_register;
            } else if(opcode == 0x1F) {
                reg = DS_register;
            } else if(opcode == 0x58) {
                reg = AX_register;
            } else if(opcode == 0x59) {
                reg = CX_register;
            } else if(opcode == 0x5A) {
                reg = DX_register;
            } else if(opcode == 0x5B) {
                reg = BX_register;
            } else if(opcode == 0x5C) {
                reg = SP_register;
            } else if(opcode == 0x5D) {
                reg = BP_register;
            } else if(opcode == 0x5E) {
                reg = SI_register;
            } else if(opcode == 0x5F) {
                reg = DI_register;
            } else if(opcode == 0x9D) {
                reg = FLAGS_register;
            }
            mylog("logs/main.log", "Instruction 0x%02X: POP %s\n", opcode, get_reg_name_string(reg));
            pop_register(reg);
            break;
        }
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid POP instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t push_reg_instr(uint8_t opcode, uint8_t *data) {
    uint8_t ret_val = 1;
    register_name_t reg;
    switch(opcode) {
        case 0x06:  // PUSH ES
        case 0x0E:  // PUSH CS
        case 0x16:  // PUSH SS
        case 0x1E:  // PUSH DS
        case 0x50:  // PUSH AX
        case 0x51:  // PUSH CX
        case 0x52:  // PUSH DX
        case 0x53:  // PUSH BX
        case 0x54:  // PUSH SP
        case 0x55:  // PUSH BP
        case 0x56:  // PUSH SI
        case 0x57:  // PUSH DI
        case 0x9C: {// PUSHF
            if(opcode == 0x06) {
                reg = ES_register;
            } else if(opcode == 0x0E) {
                reg = CS_register;
            } else if(opcode == 0x16) {
                reg = SS_register;
            } else if(opcode == 0x1E) {
                reg = DS_register;
            } else if(opcode == 0x50) {
                reg = AX_register;
            } else if(opcode == 0x51) {
                reg = CX_register;
            } else if(opcode == 0x52) {
                reg = DX_register;
            } else if(opcode == 0x53) {
                reg = BX_register;
            } else if(opcode == 0x54) {
                reg = SP_register;
            } else if(opcode == 0x55) {
                reg = BP_register;
            } else if(opcode == 0x56) {
                reg = SI_register;
            } else if(opcode == 0x57) {
                reg = DI_register;
            } else if(opcode == 0x9C) {
                reg = FLAGS_register;
            }
            push_register(get_register_value(reg));
            mylog("logs/main.log", "Instruction 0x%02X: PUSH %s\n", opcode, get_reg_name_string(reg));
            break;
        }
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid PUSH instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t esc_instr(uint8_t opcode, uint8_t *data) {
    mylog("logs/main.log", "Instruction 0xD8: ESC\n");
    return 2;
}

int16_t inc_instr(uint8_t opcode, uint8_t *data) {
    int16_t ret_val = 1;
    register_name_t reg;
    switch(opcode) {
        case 0x40:  // INC AX
        case 0x41:  // INC CX
        case 0x42:  // INC DX
        case 0x43:  // INC BX
        case 0x44:  // INC SP
        case 0x45:  // INC BP
        case 0x46:  // INC SI
        case 0x47: {// INC DI
            if(opcode == 0x40) {
                reg = AX_register;
            }else if(opcode == 0x41) {
                reg = CX_register;
            } else if(opcode == 0x42) {
                reg = DX_register;
            } else if(opcode == 0x43) {
                reg = BX_register;
            } else if(opcode == 0x44) {
                reg = SP_register;
            } else if(opcode == 0x45) {
                reg = BP_register;
            } else if(opcode == 0x46) {
                reg = SI_register;
            } else if(opcode == 0x47) {
                reg = DI_register;
            }
            uint16_t val = get_register_value(reg);
            mylog("logs/main.log", "Instruction 0x%02X: INC %s: 0x%04X => 0x%04X\n", opcode, get_reg_name_string(reg), val, val+1);
            set_register_value(reg, val+1);
            update_flags(val, 1, val+1, 2, ADD_OP);
            break;
        }
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid INC instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

int16_t dec_instr(uint8_t opcode, uint8_t *data) {
    int16_t ret_val = 1;
    register_name_t reg;
    switch(opcode) {
        case 0x48:  // DEC AX
        case 0x49:  // DEC CX
        case 0x4A:  // DEC DX
        case 0x4B:  // DEC BX
        case 0x4C:  // DEC SP
        case 0x4D:  // DEC BP
        case 0x4E:  // DEC SI
        case 0x4F: {// DEC DI
            if(opcode == 0x48) {
                reg = AX_register;
            }else if(opcode == 0x49) {
                reg = CX_register;
            } else if(opcode == 0x4A) {
                reg = DX_register;
            } else if(opcode == 0x4B) {
                reg = BX_register;
            } else if(opcode == 0x4C) {
                reg = SP_register;
            } else if(opcode == 0x4D) {
                reg = BP_register;
            } else if(opcode == 0x4E) {
                reg = SI_register;
            } else if(opcode == 0x4f) {
                reg = DI_register;
            }
            uint16_t val = get_register_value(reg);
            mylog("logs/main.log", "Instruction 0x%02X: DEC %s: 0x%04X => 0x%04X\n", opcode, get_reg_name_string(reg), val, val-1);
            set_register_value(reg, val - 1);
            update_flags(val, 1, val-1, 2, SUB_OP);
            break;
        }
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid DEC instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

void print_registers(void) {
    mylog("logs/main.log", "Registers values:\n");
    mylog("logs/main.log", "\tAX = 0x%04X, BX = 0x%04X, CX = 0x%04X, DX = 0x%04X;\n", REGS->AX, REGS->BX, REGS->CX, REGS->DX);
    mylog("logs/main.log", "\tSI = 0x%04X, DI = 0x%04X, BP = 0x%04X, SP = 0x%04X;\n", REGS->SI, REGS->DI, REGS->BP, REGS->SP);
    mylog("logs/main.log", "\tCS = 0x%04X, DS = 0x%04X, SS = 0x%04X, ES = 0x%04X;\n", REGS->CS, REGS->DS, REGS->SS, REGS->ES);
    mylog("logs/main.log", "\tIP = 0x%04X, FL = 0x%04X;\n", REGS->IP, REGS->flags);
}

int16_t inc_dec_instr(uint8_t opcode, uint8_t *data) {
    int16_t ret_val = 1;
    switch(opcode) {
        case 0xFE:
        case 0xFF: {
            operands_t operands = decode_operands(opcode, data, 1);
            ret_val += operands.num_bytes;
            uint8_t reg_field = get_register_field(data[0]);
            uint16_t res_val = 0;
            if(reg_field == 0) {    // INC
                res_val = operands.dst_val + 1;
                mylog("logs/main.log", "Instruction 0x%02X: INC %s: 0x%04X => 0x%04X\n", opcode, operands.destination, operands.dst_val, res_val);
                update_flags(res_val, 1, res_val, operands.width, ADD_OP);
            } else if (reg_field == 1) {    // DEC
                res_val = operands.dst_val - 1;
                mylog("logs/main.log", "Instruction 0x%02X: DEC %s: 0x%04X => 0x%04X\n", opcode, operands.destination, operands.dst_val, res_val);
                update_flags(res_val, 1, res_val, operands.width, SUB_OP);
            } else {
                printf("ERROR: Invalid INC/DEC operation: 0x%02X, reg_field = %d\n", opcode, reg_field);
                REGS->invalid_operations ++;
            }
            if (operands.dst_type == 0) {   // Reg mode
                set_register_value(operands.dst.register_name, res_val);
            } else {    // Memory mode
                mem_write(operands.dst.address, res_val, operands.width);
            }
            break;
        }
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid INC/DEC instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t xchg_instr(uint8_t opcode, uint8_t *data) {
    uint8_t ret_val = 1;
    switch(opcode) {
        case 0x86:      // XCHG REG8, REG8/MEM8: [0x86, MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x87: {    // XCHG REG16, REG16/MEM16: [0x86, MOD REG R/M (DISP-LO), (DISP-HI)]
            operands_t operands = decode_operands(opcode, data, 0);
            ret_val += operands.num_bytes;
            if (operands.src_type == 0) {   // Reg mode
                set_register_value(operands.src.register_name, operands.dst_val);
            } else {    // Memory mode
                mem_write(operands.src.address, operands.dst_val, operands.width);
            }
            set_register_value(operands.dst.register_name, operands.src_val);
            mylog("logs/main.log", "Instruction 0x%02X: XCHG %s (0x%04X), %s (0x%04X)\n", opcode, operands.destination, operands.dst_val, operands.source, operands.src_val);
            break;
        }
        case 0x90:  // XCHG AX, AX (NOP))
        case 0x91:  // XCHG AX, CX
        case 0x92:  // XCHG AX, DX
        case 0x93:  // XCHG AX, BX
        case 0x94:  // XCHG AX, SP
        case 0x95:  // XCHG AX, BP
        case 0x96:  // XCHG AX, SI
        case 0x97: {  // XCHG AX, DI
            register_name_t r1 = AX_register;
            register_name_t r2 = AX_register;
            if(opcode == 0x90) {  // XCHG AX, AX (NOP))
                    r2 = AX_register;
                    mylog("logs/main.log", "Instruction 0x%02X: XCHG AX, AX (NOP)\n", opcode);
            } else if(opcode == 0x91) {  // XCHG AX, CX
                    r2 = CX_register;
                    mylog("logs/main.log", "Instruction 0x%02X: XCHG AX, CX\n", opcode);
            } else if(opcode == 0x92) {  // XCHG AX, DX
                    r2 = DX_register;
                    mylog("logs/main.log", "Instruction 0x%02X: XCHG AX, DX\n", opcode);
            } else if(opcode == 0x93) {  // XCHG AX, BX
                    r2 = BX_register;
                    mylog("logs/main.log", "Instruction 0x%02X: XCHG AX, BX\n", opcode);
            } else if(opcode == 0x94) {  // XCHG AX, SP
                    r2 = SP_register;
                    mylog("logs/main.log", "Instruction 0x%02X: XCHG AX, SP\n", opcode);
            } else if(opcode == 0x95) {  // XCHG AX, BP
                    r2 = BP_register;
                    mylog("logs/main.log", "Instruction 0x%02X: XCHG AX, BP\n", opcode);
            } else if(opcode == 0x96) {  // XCHG AX, SI
                    r2 = SI_register;
                    mylog("logs/main.log", "Instruction 0x%02X: XCHG AX, SI\n", opcode);
            } else if(opcode == 0x97) {  // XCHG AX, DI
                    r2 = DI_register;
                    mylog("logs/main.log", "Instruction 0x%02X: XCHG AX, DI\n", opcode);
            }
            set_register_value(AX_register, get_register_value(r2));
            set_register_value(r2, get_register_value(r1));
            break;
        }
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid INC/DEC instruction: 0x%02X\n", opcode);
    }
    return ret_val;
}

uint8_t string_instr(uint8_t opcode, uint8_t *data) {
    uint8_t opcode_len = 1;
    switch(opcode) {
        case 0xAA: {  // STOS DEST-STR8
            opcode_len = 1;
            break;
        }
        case 0xAB: {  // STOS DEST-STR16
            opcode_len = 1;
            break;
        }
        case 0xA5: {  // MOVS DEST-STR16, SRC-STR16
            opcode_len = 1;
            break;
        }
        case 0xAC: {  // LODS SRC-STR8
            opcode_len = 1;
            break;
        }
        case 0xAD: {  // LODS SRC-STR16
            opcode_len = 1;
            break;
        }
    }
    if(get_prefix(REPE) || get_prefix(REPNE)) {
        uint16_t cx = get_register_value(CX_register);
        if(cx == 0) {
            set_prefix(REPE, 0);
            set_prefix(REPNE, 0);
            return opcode_len;
        }
        set_register_value(CX_register, cx - 1);
    }
    switch(opcode) {
        case 0xA5: {  // MOVS DEST-STR16, SRC-STR16
            uint32_t dst_addr = get_addr(DS_register, get_register_value(DI_register));
            uint32_t src_addr = get_addr(DS_register, get_register_value(SI_register));
            uint16_t val = mem_read(src_addr, 2);
            mem_write(dst_addr, val, 2);
            if(get_flag(DF)) {
                set_register_value(DI_register, get_register_value(DI_register) - 1);
                set_register_value(SI_register, get_register_value(SI_register) - 1);
            } else {
                set_register_value(DI_register, get_register_value(DI_register) + 1);
                set_register_value(SI_register, get_register_value(SI_register) + 1);
            }
            mylog("logs/main.log", "Instruction 0x%02X: MOVS DEST-STR16, SRC-STR16 (0x%08X <= 0x%04X @ 0x%08X)\n", opcode, dst_addr, val, src_addr);
            break;
        }
        case 0xAA: {  // STOS DEST-STR8
            uint32_t addr = get_addr(DS_register, get_register_value(DI_register));
            mem_write(addr, get_register_value(AL_register), 1);
            if(get_flag(DF)) {
                set_register_value(DI_register, get_register_value(DI_register) - 1);
            } else {
                set_register_value(DI_register, get_register_value(DI_register) + 1);
            }
            mylog("logs/main.log", "Instruction 0x%02X: STOS DEST-STR8 (0x%08X)\n", opcode, addr);
            break;
        }
        case 0xAB: {  // STOS DEST-STR16
            uint32_t addr = get_addr(DS_register, get_register_value(DI_register));
            mem_write(addr, get_register_value(AX_register), 2);
            if(get_flag(DF)) {
                set_register_value(DI_register, get_register_value(DI_register) - 2);
            } else {
                set_register_value(DI_register, get_register_value(DI_register) + 2);
            }
            mylog("logs/main.log", "Instruction 0x%02X: STOS DEST-STR16 (0x%08X)\n", opcode, addr);
            break;
        }
        case 0xAC: {  // LODS SRC-STR8
            uint32_t addr = get_addr(DS_register, get_register_value(SI_register));
            set_register_value(AL_register, mem_read(addr, 1));
            if(get_flag(DF)) {
                set_register_value(SI_register, get_register_value(SI_register) - 1);
            } else {
                set_register_value(SI_register, get_register_value(SI_register) + 1);
            }
            mylog("logs/main.log", "Instruction 0x%02X: LODS SRC-STR8 (addr = 0x%08X, value = 0x%04X)\n", opcode, addr, get_register_value(AL_register));
            break;
        }
        case 0xAD: {  // LODS SRC-STR16
            uint32_t addr = get_addr(DS_register, get_register_value(SI_register));
            set_register_value(AX_register, mem_read(addr, 2));
            if(get_flag(DF)) {
                set_register_value(SI_register, get_register_value(SI_register) - 2);
            } else {
                set_register_value(SI_register, get_register_value(SI_register) + 2);
            }
            mylog("logs/main.log", "Instruction 0x%02X: LODS SRC-STR16 (0x%08X)\n", opcode, addr);
            break;
        }
        default:
            REGS->invalid_operations ++;
            printf("Error: Invalid string operation: 0x%02X\n", opcode);
    }
    if(get_prefix(REPE) || get_prefix(REPNE)) {
        return 0;
    } else {
        return opcode_len;
    }
}

int16_t process_instruction(uint8_t * memory) {
    mylog("logs/main.log", "===============================================================\n");
    mylog("logs/main.log", "Step %d, processing bytes: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X:\n",
           REGS->ticks, memory[0], memory[1], memory[2], memory[3], memory[4], memory[5]);
    print_registers();
    mylog("logs/short.log", "Step: %d, IP: 0x%04X, data: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
          REGS->ticks, REGS->IP, memory[0], memory[1], memory[2], memory[3], memory[4], memory[5]);
    int16_t ret_val = 1;
    switch(memory[0]) {
        case 0x00:  // ADD REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x01:  // ADD REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x02:  // ADD REG8, REG8/MEM8: [0x02, MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x03:  // ADD REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x04:  // ADD AL, IMMED8           [DATA-8]
        case 0x05:  // ADD AX, immed16          [0x05, DATA-LO, DATA-HI]
            ret_val = add_instr(memory[0], &memory[1]);
            break;
        case 0x06:  // PUSH ES
            ret_val = push_reg_instr(memory[0], &memory[1]);
            break;
        case 0x07:  // POP ES
            ret_val = pop_reg_instr(memory[0], &memory[1]);
            break;
        case 0x08:  // OR REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x09:  // OR REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x0A:  // OR REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x0B:  // OR REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x0C:  // OR AL, IMMED8           [DATA-8]
        case 0x0D:  // OR AX, immed16          [DATA-LO, DATA-HI]
            ret_val = or_instr(memory[0], &memory[1]);
            break;
        case 0x0E:  // PUSH CS
            ret_val = push_reg_instr(memory[0], &memory[1]);
            break;
        case 0x0F:  // INVALID_INSTRUCTION;
            REGS->invalid_operations ++;
            printf("Invalid instruction: 0x%02X\n", memory[0]);
            break;
        case 0x10:  // ADC REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x11:  // ADC REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x12:  // ADC REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x13:  // ADC REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x14:  // ADC AL, IMMED8           [DATA-8]
        case 0x15:  // ADC AX, immed16          [DATA-LO, DATA-HI]
            ret_val = add_instr(memory[0], &memory[1]);
            break;
        case 0x16:  // PUSH SS
            ret_val = push_reg_instr(memory[0], &memory[1]);
            break;
        case 0x17:  // POP SS
            ret_val = pop_reg_instr(memory[0], &memory[1]);
            break;
        case 0x18:  // SBB REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x19:  // SBB REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x1A:  // SBB REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x1B:  // SBB REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x1C:  // SBB AL, IMMED8           [DATA-8]
        case 0x1D:  // SBB AX, immed16          [DATA-LO, DATA-HI]
            ret_val = sub_instr(memory[0], &memory[1]);
            break;
        case 0x1E:  // PUSH DS
            ret_val = push_reg_instr(memory[0], &memory[1]);
            break;
        case 0x1F:  // POP DS
            ret_val = pop_reg_instr(memory[0], &memory[1]);
            break;
        case 0x20:  // AND REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x21:  // AND REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x22:  // AND REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x23:  // AND REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x24:  // AND AL, IMMED8           [DATA-8]
        case 0x25:  // AND AX, immed16          [DATA-LO, DATA-HI]
            ret_val = and_instr(memory[0], &memory[1]);
            break;
        case 0x26:  // ES:
            mylog("logs/main.log", "Instruction 0x26: ES (override segment)\n");
            set_register_value(override_segment, ES_register);
            break;
        case 0x27:  // DAA
            ret_val = adj_instr(memory[0], &memory[1]);
            break;
        case 0x28:  // SUB REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x29:  // SUB REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x2A:  // SUB REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x2B:  // SUB REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x2C:  // SUB AL, IMMED8           [DATA-8]
        case 0x2D:  // SUB AX, immed16          [DATA-LO, DATA-HI]
            ret_val = sub_instr(memory[0], &memory[1]);
            break;
        case 0x2E:  // CS:
            mylog("logs/main.log", "Instruction 0x2E: CS (override segment)\n");
            set_register_value(override_segment, CS_register);
            break;
        // case 0x2F:  // DAS
        //     das_op();
        //     REGS->IP += 1;
        //     break;
        case 0x30:  // XOR REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x31:  // XOR REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x32:  // XOR REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x33:  // XOR REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x34:  // XOR AL, IMMED8           [DATA-8]
        case 0x35:  // XOR AX, immed16          [DATA-LO, DATA-HI]
            ret_val = xor_instr(memory[0], &memory[1]);
            break;
        case 0x36:  // SS:
            mylog("logs/main.log", "Instruction 0x36: SS (override segment)\n");
            set_register_value(override_segment, SS_register);
            break;
            break;
        // case 0x37:  // AAA
        //     aaa_op();
        //     REGS->IP += 1;
        //     break;
        case 0x38:  // CMP REG8/MEM8, REG8;     [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x39:  // CMP REG16/MEM16, REG16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x3A:  // CMP REG8, REG8/MEM8      [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x3B:  // CMP REG16, REG16/MEM16   [MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x3C:  // CMP AL, IMMED8: [0x3C, DATA-8]
        case 0x3D:  // CMP AX, immed16: [0x3D, DATA-LO, DATA-HI]
            ret_val = cmp_instr(memory[0], &memory[1]);
            break;
        case 0x3E:  // DS:
            mylog("logs/main.log", "Instruction 0x3E: DS (override segment)\n");
            set_register_value(override_segment, DS_register);
            break;
            break;
        // case 0x3F:  // AAS
        //     aas_op();
        //     REGS->IP += 1;
        //     break;
        case 0x40:  // INC AX
        case 0x41:  // INC CX
        case 0x42:  // INC DX
        case 0x43:  // INC BX
        case 0x44:  // INC SP
        case 0x45:  // INC BP
        case 0x46:  // INC SI
        case 0x47:  // INC DI
            ret_val = inc_instr(memory[0], &memory[1]);
            break;
        case 0x48:  // DEC AX
        case 0x49:  // DEC CX
        case 0x4A:  // DEC DX
        case 0x4B:  // DEC BX
        case 0x4C:  // DEC SP
        case 0x4D:  // DEC BP
        case 0x4E:  // DEC SI
        case 0x4F:  // DEC DI
            ret_val = dec_instr(memory[0], &memory[1]);
            break;
        case 0x50:  // PUSH AX
        case 0x51:  // PUSH CX
        case 0x52:  // PUSH DX
        case 0x53:  // PUSH BX
        case 0x54:  // PUSH SP
        case 0x55:  // PUSH BP
        case 0x56:  // PUSH SI
        case 0x57:  // PUSH DI
            ret_val = push_reg_instr(memory[0], &memory[1]);
            break;
        case 0x58:  // POP AX
        case 0x59:  // POP CX
        case 0x5A:  // POP DX
        case 0x5B:  // POP BX
        case 0x5C:  // POP SP
        case 0x5D:  // POP BP
        case 0x5E:  // POP SI
        case 0x5F:  // POP DI
            ret_val = pop_reg_instr(memory[0], &memory[1]);
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
            // INVALID_INSTRUCTION;
            REGS->invalid_operations ++;
            printf("Invalid instruction: 0x%02X\n", memory[0]);
            break;
        case 0x70:
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0x71:
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0x72:  // JB/JNAE/SHORT-LABEL JC: [0x72, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0x73:  // JNB/JAE SHORT-LABEL JNB: [0x73, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0x74:  // JE/JZ SHORT-LABEL: [0x74, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0x75:  // JNE/JNZ SHORT LABEL: [0x75, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0x76:  // JBE/JNA SHORT-LABE: [0x76, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        // case 0x77:
        //     if(!jbe_op())
        //         REGS->IP = memory[1];
        //     break;
        case 0x78:  // JS SHORT LABEL: [0x78, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0x79:  // JNS SHORT LABEL: [0x79, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0x7A:  // JP/JPE SHORT-LABEL: [0x7A, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0x7B:  // JNP/JPO SHORT LABEL: [0x7B, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0x7C:   // JL/JNGE SHORT-LABEL: [0x7F, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        // case 0x7D:   // JNL/JGE SHORT-LABEL: [0x7F, IP-INC8]
        //     if(!jl_op())
        //         REGS->IP = memory[1];
        //     break;
        // case 0x7E:   // JLE/JNG SHORT-LABEL: [0x7F, IP-INC8]
        //     if(jle_op())
        //         REGS->IP = memory[1];
        //     break;
        // case 0x7F:   // JNLE/JG SHORT-LABEL: [0x7F, IP-INC8]
        //     if(!jle_op())
        //         REGS->IP = memory[1];
        //     break;
        case 0x80:
            // 8-bit operations
            switch(get_register_field(memory[1])) {
                case 0: // ADD REG8/MEM8, IMMED8: [0x80, MOD 000 R/M, (DISP-LO), (DISP-HI), DATA-8]
                    printf("ERROR: Unimplemented ADD (0x80) instruction!\n");
                    REGS->invalid_operations ++;
                    break;
                case 1: // OR REG8/MEM8, IMMED8: [0x80, MOD 001 R/M, (DISP-LO), (DISP-HI), DATA-8]
                    ret_val = or_instr(memory[0], &memory[1]);
                    break;
                case 2: // ADC REG8/MEM8, IMMED8: [0x80, MOD 010 R/M, (DISP-LO), (DISP-HI), DATA-8]
                    printf("ERROR: Unimplemented ADC (0x80) instruction!\n");
                    REGS->invalid_operations ++;
                    break;
                case 3: // SBB REG8/MEM8, IMMED8: [0x80, MOD 011 R/M, (DISP-LO), (DISP-HI), DATA-8]
                    ret_val = sub_instr(memory[0], &memory[1]);
                    break;
                case 4: // AND REG8/MEM8, IMMED8: [0x80, MOD 100 R/M, (DISP-LO), (DISP-HI), DATA-8]
                    printf("ERROR: Unimplemented AND (0x80) instruction!\n");
                    REGS->invalid_operations ++;
                    break;
                case 5: // SUB REG8/MEM8, IMMED8: [0x80, MOD 101 R/M, (DISP-LO), (DISP-HI), DATA-8]
                    printf("ERROR: Unimplemented SUB (0x80) instruction!\n");
                    REGS->invalid_operations ++;
                    break;
                case 6: // XOR REG8/MEM8, IMMED8: [0x80, MOD 110 R/M, (DISP-LO), (DISP-HI), DATA-8]
                    printf("ERROR: Unimplemented XOR (0x80) instruction!\n");
                    REGS->invalid_operations ++;
                    break;
                case 7: // CMP REG8/MEM8, IMMED8: [0x80, MOD 111 R/M, (DISP-LO), (DISP-HI), DATA-8]
                    ret_val = sub_instr(memory[0], &memory[1]);
                    break;
            }
            break;
        case 0x81:
            // 16-bit operations
            switch(get_register_field(memory[1])) {
                case 0: // ADD REG16/MEM16,IMMED16: [0x81, MOD 000 R/M, (DISP-LO), (DISP-HI), DATA-LO, DATA-HI]
                    ret_val = add_instr(memory[0], &memory[1]);
                    break;
                case 1: // OR REG16/MEM16,IMMED16: [0x81, MOD 001 R/M, (DISP-LO), (DISP-HI), DATA-LO, DATA-HI]
                    printf("ERROR: Unimplemented OR (0x81) instruction!\n");
                    REGS->invalid_operations ++;
                    break;
                case 2: // ADC REG16/MEM16,IMMED16: [0x81, MOD 010 R/M, (DISP-LO), (DISP-HI), DATA-LO, DATA-HI]
                    printf("ERROR: Unimplemented ADC (0x81) instruction!\n");
                    REGS->invalid_operations ++;
                    break;
                case 3: // SBB REG16/MEM16,IMMED16: [0x81, MOD 011 R/M, (DISP-LO), (DISP-HI), DATA-LO, DATA-HI]
                    ret_val = sub_instr(memory[0], &memory[1]);
                    break;
                case 4: // AND REG16/MEM16,IMMED16: [0x81, MOD 100 R/M, (DISP-LO), (DISP-HI), DATA-LO, DATA-HI]
                    printf("ERROR: Unimplemented AND (0x81) instruction!\n");
                    REGS->invalid_operations ++;
                    break;
                case 5: // SUB REG16/MEM16,IMMED16: [0x81, MOD 101 R/M, (DISP-LO), (DISP-HI), DATA-LO, DATA-HI]
                    ret_val = sub_instr(memory[0], &memory[1]);
                    break;
                case 6: // XOR REG16/MEM16,IMMED16: [0x81, MOD 110 R/M, (DISP-LO), (DISP-HI), DATA-LO, DATA-HI]
                    printf("ERROR: Unimplemented XOR (0x81) instruction!\n");
                    REGS->invalid_operations ++;
                    break;
                case 7: // CMP REG16/MEM16,IMMED16: [0x81, MOD 111 R/M, (DISP-LO), (DISP-HI), DATA-LO, DATA-HI]
                    ret_val = sub_instr(memory[0], &memory[1]);
                    break;
            }
            break;
        // case 0x82:
        //     // 8-bit operations
        //     switch(get_register_field(memory[1])) {
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
        case 0x83:  // 16-bit operations
            switch(get_register_field(memory[1])) {
                case 0: // ADD REG16/MEM16, IMMED8: [0x83, MOD 000 RIM, (DISP-LO),(DISP-HI), DATA-SX]
                    ret_val = add_instr(memory[0], &memory[1]);
                    break;
                case 1:
                    printf("ERROR: Invalid instruction: 0x83 REG = 1\n");
                    REGS->invalid_operations ++;
                    break;
                case 2: // ADC REG16/MEM16, IMMED8: [0x83, MOD 010 RIM, (DISP-LO),(DISP-HI), DATA-SX]
                    printf("ERROR: Unimplemented ADC (0x83) instruction!\n");
                    REGS->invalid_operations ++;
                    break;
                case 3: // SBB REG16/MEM16, IMMED8: [0x83, MOD 011 RIM, (DISP-LO),(DISP-HI), DATA-SX]
                    ret_val = sub_instr(memory[0], &memory[1]);
                    break;
                case 4:
                    printf("ERROR: Invalid instruction: 0x83 REG = 4\n");
                    REGS->invalid_operations ++;
                    break;
                case 5: // SUB REG16/MEM16, IMMED8: [0x83, MOD 101 RIM, (DISP-LO),(DISP-HI), DATA-SX]
                    ret_val = sub_instr(memory[0], &memory[1]);
                    break;
                case 6: // Invalid intruction
                    printf("ERROR: Invalid instruction: 0x83 REG = 6\n");
                    REGS->invalid_operations ++;
                    break;
                case 7: // CMP REG16/MEM16, IMMED8: [0x83, MOD 111 RIM, (DISP-LO),(DISP-HI), DATA-SX]
                    ret_val = sub_instr(memory[0], &memory[1]);
                    break;
            }
            break;
        // case 0x84:
        //     REGS->IP += test_8b_op(memory[1], &memory[2]);
        //     break;
        // case 0x85:
        //     REGS->IP += test_16b_op(memory[1], &memory[2]);
        //     break;
        case 0x86:  // XCHG REG8, REG8/MEM8: [0x86, MOD REG R/M (DISP-LO), (DISP-HI)]
        case 0x87:  // XCHG REG16, REG16/MEM16: [0x86, MOD REG R/M (DISP-LO), (DISP-HI)]
            ret_val = xchg_instr(memory[0], &memory[1]);
            break;
        case 0x88:  // MOV REG8/MEM8, REG8: 0x88, MOD REG R/M (DISP-LO), (DISP-HI)]
        case 0x89:  // MOV REG16/MEM16, REG16: [0x89, MOD REG R/M, (DISP-LO),(DISP-HI)]
        case 0x8A:  // MOV REG8, REG8/MEM8: [0x8A, MOD REG R/M, (DISP-LO),(DISP-HI)]
        case 0x8B:  // MOV REG16, REG16/MEM16: [0x8B, MOD REG R/M, (DISP-LO), (DISP-HI)]
        case 0x8C:  // MOV REG16/MEM16, SEGREG: [0x8C, MOD 0SR R/M, (DISP-LO), (DISP-HI)]
            // reg_field: Segment register code: OO=ES, 01=CS, 10=SS, 11 =DS
            ret_val = mov_instr(memory[0], &memory[1]);
            break;
        // case 0x8D:  // LEA REG16, MEM16
        //     REGS->IP += lea_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
        //     break;
        case 0x8E:  // MOV SEGREG, REG16/MEM16: [0x8C, MOD 0SR R/M, (DISP-LO), (DISP-HI)]
            // reg_field: Segment register code: OO=ES, 01=CS, 10=SS, 11 =DS
            ret_val = mov_instr(memory[0], &memory[1]);
            break;
        // case 0x8F:
        //     if(0 == get_register_field(memory[1]))  // POP REG16/MEM16
        //         REGS->IP += pop_op(memory[1], &memory[2]);  // DISP-LO, DISP-HI
        //     else
        //         INVALID_INSTRUCTION;
        //     break;
        case 0x90:  // XCHG AX, AX (NOP))
        case 0x91:  // XCHG AX, CX
        case 0x92:  // XCHG AX, DX
        case 0x93:  // XCHG AX, BX
        case 0x94:  // XCHG AX, SP
        case 0x95:  // XCHG AX, BP
        case 0x96:  // XCHG AX, SI
        case 0x97:  // XCHG AX, DI
            ret_val = xchg_instr(memory[0], &memory[1]);
            break;
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
        case 0x9C:  // PUSHF
            ret_val = push_reg_instr(memory[0], &memory[1]);
            break;
        case 0x9D:  // POPF
            ret_val = pop_reg_instr(memory[0], &memory[1]);
            break;
        case 0x9E:  // SAHF
            ret_val = sahf_instr();
            break;
        case 0x9F:  // LAHF
            ret_val = lahf_instr();
            break;
        case 0xA0:  // MOV AL, MEM8: [0xA0, DISP-LO, DISP-HI]
        case 0xA1:  // MOV AX, MEM16: [0xA1, DISP-LO, DISP-HI]
        case 0xA2:  // MOV MEM8, AL: [0xA2, DISP-LO, DISP-HI]
        case 0xA3:  // MOV MEM8, AX: [0xA3, DISP-LO, DISP-HI]
            ret_val = mov_instr(memory[0], &memory[1]);
            break;
        // case 0xA4:  // MOVS DEST-STR8, SRC-STR8
        //     REGS->IP += mov_str_op(memory[1], &memory[2]);
        //     break;
        case 0xA5:  // MOVS DEST-STR16, SRC-STR16
            ret_val = string_instr(memory[0], &memory[1]);
            break;
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
        case 0xAA:  // STOS DEST-STR8
            ret_val = string_instr(memory[0], &memory[1]);
            break;
        case 0xAB:  // STOS DEST-STR16
            ret_val = string_instr(memory[0], &memory[1]);
            break;
        case 0xAC:  // LODS DEST-STR8
            ret_val = string_instr(memory[0], &memory[1]);
            break;
        case 0xAD:  // LODS DEST-STR16
            ret_val = string_instr(memory[0], &memory[1]);
            break;
        // case 0xAE:  // SCAS DEST-STR8
        //     REGS->IP += stos_str_op(memory[1], &memory[2]);
        //     break;
        // case 0xAF:  // SCAS DEST-STR16
        //     REGS->IP += stos_str_op(memory[1], &memory[2]);
        //     break;
        case 0xB0:  // MOV AL, IMMED8: [0xB0, immed8]
        case 0xB1:  // MOV CL, IMMED8: [0xB1, immed8]
        case 0xB2:  // MOV DL, IMMED8: [0xB2, immed8]
        case 0xB3:  // MOV BL, IMMED8: [0xB3, immed8]
        case 0xB4:  // MOV AH, IMMED8: [0xB4, immed8]
        case 0xB5:  // MOV CH, IMMED8: [0xB5, immed8]
        case 0xB6:  // MOV DH, IMMED8: [0xB6, immed8]
        case 0xB7:  // MOV BH, IMMED8: [0xB7, immed8]
        case 0xB8:  // MOV AX, IMMED16 [0xB8, DATA-LO, DATA-HI]
        case 0xB9:  // MOV CX, IMMED16: [0xB9, DATA-LO, DATA-HI]
        case 0xBA:  // MOV DX, IMMED16: [0xBA, DATA-HI, DATA-LO]
        case 0xBB:  // MOV BX, IMMED16: [0xBB, DATA-LO, DATA-HI]
        case 0xBC:  // MOV SP, IMMED16: [0xBC, DATA-LO, DATA-HI]
        case 0xBD:  // MOV BP, IMMED16: [0xBD, DATA-LO, DATA-HI]
        case 0xBE:  // MOV SI, IMMED16: [0xBE, DATA-LO, DATA-HI]
        case 0xBF:  // MOV DI, IMMED16: [0xBF, DATA-LO, DATA-HI]
            ret_val = mov_instr(memory[0], &memory[1]);
            break;
        case 0xC0:
            // INVALID_INSTRUCTION;
            REGS->invalid_operations ++;
            printf("Invalid instruction: 0x%02X\n", memory[0]);
            break;
        case 0xC1:
            // INVALID_INSTRUCTION;
            REGS->invalid_operations ++;
            printf("Invalid instruction: 0x%02X\n", memory[0]);
            break;
        // case 0xC2:  // RET IMMED16 (intrasegment)
        //     REGS->IP += ret_op(memory[1], &memory[2]);  // DATA-LO, DATA-HI
        //     break;
        case 0xC3:  // RET (intrasegment)
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        // case 0xC4:  // LES REG16, MEM16
        //     REGS->IP += les_op(memory[1], &memory[2]);  // MOD REG R/M, DISP-LO, DISP-HI
        //     break;
        // case 0xC5:  // LDS REG16, MEM16
        //     REGS->IP += lds_op(memory[1], &memory[2]);  // MOD REG R/M, DISP-LO, DISP-HI
        //     break;
        case 0xC6:  // MOV MEM8, IMMED8: [0xC6, MOD 000 R/M, (DISP-LO), (DISP-HI), DATA-8]
            ret_val = mov_instr(memory[0], &memory[1]);
            break;
        case 0xC7:  // MOV MEM16, IMMED16
            ret_val = mov_instr(memory[0], &memory[1]);
            break;
        case 0xC8:    // INVALID_INSTRUCTION;
            REGS->invalid_operations ++;
            printf("Invalid instruction: 0x%02X\n", memory[0]);
            break;
        case 0xC9:     // INVALID_INSTRUCTION;
            REGS->invalid_operations ++;
            printf("Invalid instruction: 0x%02X\n", memory[0]);
            break;
        // case 0xCA:  // RET IMMED16 (intersegment)
        //     REGS->IP += ret_op(memory[1], &memory[2]);  // DATA-LO, DATA-HI
        //     break;
        // case 0xCB:  // RET (intersegment)
        //     REGS->IP += ret_op(memory[1], &memory[2]);
        //     break;
        case 0xCC:  // INT 3
            set_int_vector(3);
            ret_val = 1;
            break;
        case 0xCD:  // INT IMMED8
            set_int_vector(memory[1]);
            ret_val = 2;
            break;
        case 0xCE:  // INT0
            set_int_vector(0);
            ret_val = 1;
            break;
        case 0xCF:  // IRET
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0xD0:  // 8-bit operations
        case 0xD1:  // 16-bit operations
        case 0xD2:  // 8-bit operations
        case 0xD3:  // 16-bit operations
            ret_val = shift_instr(memory[0], &memory[1]);
            break;
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
        case 0xD6:
            // INVALID_INSTRUCTION;
            REGS->invalid_operations ++;
            printf("Invalid instruction: 0x%02X\n", memory[0]);
            break;
        // case 0xD7:  // XLAT SOURCE-TABLE
        //     REGS->IP += xlat_op();
        //     break;
        case 0xD8:  // ESC OPCODE, SOURCE
        // case 0xD9:
        // case 0xDA:
        // case 0xDB:
        // case 0xDC:
        // case 0xDD:
        // case 0xDE:
        // case 0xDF:
            //  When the 8086 encounters an ESC instruction with two register operands (i.e. mod = 11),
            // it performs a nop. When the processor encounters an ESC instruction with a memory operand,
            // a read cycle is performed from the address indicated by the memory operand and the result is discarded.
            ret_val = esc_instr(memory[0], &memory[1]);
            break;
        case 0xE0:  // LOOPNE/LOOPNZ SHORT-LABEL: [0xE0, IP-INC8]
            // LOOPNE decrements ecx and checks that ecx is not zero and ZF is not set - if these
            // conditions are met, it jumps at label, otherwise falls through
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0xE1:  // LOOPE/LOOPZ SHORT-LABEL
            // LOOPE decrements ecx and checks that ecx is not zero and ZF is set - if these
            // conditions are met, it jumps at label, otherwise falls through
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0xE2:  // LOOP SHORT-LABEL
            // LOOP decrements ecx and checks if ecx is not zero, if that 
            // condition is met it jumps at specified label, otherwise falls through
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0xE3:  // JPXZ
            // JCXZ (Jump if ECX Zero) branches to the label specified in the instruction if it finds a value of zero in ECX
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0xE4:  // IN AL, IMMED8
            mylog("logs/main.log", "Instruction 0xE4: IN AL IMMED8, immed8 = 0x%02X;\n", memory[1]);
            set_register_value(AL_register, io_read(memory[1], 1));
            ret_val = 2;
            break;
        // case 0xE5:  // IN AX, IMMED8
        //     REGS->AX = get_io_16bit(memory[1]);  // DATA-8
        //     REGS->IP += 2;
        //     break;
        case 0xE6:  // OUT AL, IMMED8
            // Move the content of the AL register to the io port specified in the immed8 field
            mylog("logs/main.log", "Instruction 0xE6: OUT AL IMMED8, immed8 = 0x%02X;\n", memory[1]);
            io_write(memory[1], get_register_value(AL_register), 1); // DATA-8
            ret_val += 1;
            break;
        // case 0xE7:  // OUT AX, IMMED8
        //     set_io_16bit(memory[1], REGS->AX); // DATA-8
        //     REGS->IP += 2;
        //     break;
        case 0xE8:  // CALL NEAR-PROC: [0xE8, IP-ING-LO, IP-INC-HI]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0xE9:  // JMP NEAR-LABEL: [0xE9, IP-INC-LO, IP-INC-HI]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0xEA:  // JMP FAR-LABEL: [0xEA, IP-LO, IP-HI, CS-LO, CS-HI]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0xEB:  // JMP SHORT-LABEL: [0xEB, IP-INC8]
            ret_val = jmp_instr(memory[0], &memory[1]);
            break;
        case 0xEC:  // IN AL, DX
            mylog("logs/main.log", "Instruction 0xEC: IN AL DX\n");
            set_register_value(AL_register, io_read(get_register_value(DX_register), 1));
            break;
        case 0xED:  // IN AX, DX
            set_register_value(AX_register, io_read(get_register_value(DX_register), 2));
            break;
        case 0xEE:  // OUT AL, DX
            mylog("logs/main.log", "Instruction 0xEE: OUT DX AL\n");
            io_write(get_register_value(DX_register), get_register_value(AL_register), 1); // DATA-8
            break;
        // case 0xEF:  // OUT AX, DX
        //     set_io_16bit(REGS->DX, REGS->AX); // DATA-8
        //     REGS->IP += 1;
        //     break;
        // case 0xF0:  // LOCK (prefix)
        //     assert_LOCK_pin();
        //     REGS->IP += 1;
        //     break;
        case 0xF1:  // INVALID_INSTRUCTION;
            REGS->invalid_operations ++;
            printf("Invalid instruction: 0x%02X\n", memory[0]);
            break;
        case 0xF2:  // REPNE/REPNZ
            printf("Instruction 0xF2: REPNE/REPNZ, setting prefix\n");
            set_prefix(REPNE, 1);
            break;
        case 0xF3:  // REP/REPE/REPZ
            printf("Instruction 0xF3: REP/REPE/REPZ, setting prefix\n");
            set_prefix(REPE, 1);
            break;
        case 0xF4:  // HLT
            // halts the CPU until the next external interrupt is fired
            REGS->halt = 1;
            break;
        // case 0xF5:  // CMC
        //     // Inverts the CF flag
        //     if(REGS->flags.CF) {
        //         REGS->flags.CF = 0;
        //     } else {
        //         REGS->flags.CF = 1;
        //     }
        //     REGS->IP += 1;
        //     break;
        case 0xF6:
            switch(get_register_field(memory[1])) {
                case 0: // TEST REG8/MEM8, IMMED8: [0xF6, MOD 000 R/M, DISP-LO, DISP-HI, DATA-8]
                    // TEST sets the zero flag, ZF, when the result of the AND operation is zero.
                    // If two operands are equal, their bitwise AND is zero when both are zero.
                    // TEST also sets the sign flag, SF, when the most significant bit is set in
                    // the result, and the parity flag, PF, when the number of set bits is even.
                    
                    ret_val = and_instr(memory[0], &memory[1]);
                    // printf("Error: Unimplemented TEST instruction: 0x%02X\n", memory[0]);
                    // REGS->invalid_operations ++;
                    break;
                case 1:
                    printf("Error: Invalid Instruction: 0x%02X 001\n", memory[0]);
                    REGS->invalid_operations ++;
                    break;
                case 2: // NOT REG8/MEM8: [0xF6, MOD 010 R/M, DISP-LO, DISP-HI]
                    printf("Error: Unimplemented NOT instruction: 0x%02X\n", memory[0]);
                    REGS->invalid_operations ++;
                    break;
                case 3: // NEG REG8/MEM8: [0xF6, MOD 011 R/M, DISP-LO, DISP-HI]
                    // subtracts the operand from zero and stores the result in the same operand.
                    // The NEG instruction affects the carry, overflow, sign, zero, and parity flags according to the result.
                    printf("Error: Unimplemented NEG instruction: 0x%02X\n", memory[0]);
                    REGS->invalid_operations ++;
                    break;
                case 4: // MUL REG8/MEM8: [0xF6, MOD 100 R/M, DISP-LO, DISP-HI]
                    // the source operand (in a general-purpose register or memory location) is multiplied by the value in the AL, AX, EAX, or RAX
                    // register (depending on the operand size) and the product (twice the size of the input operand) is stored in the
                    // AX, DX:AX, EDX:EAX, or RDX:RAX registers, respectively
                    printf("Error: Unimplemented MUL instruction: 0x%02X\n", memory[0]);
                    REGS->invalid_operations ++;
                    break;
                case 5: // IMUL REG8/MEM8 (signed): [0xF6, MOD 101 R/M, DISP-LO, DISP-HI]
                    ret_val = mul_instr(memory[0], &memory[1]);
                    break;
                case 6: // DIV REG8/MEM8: [0xF6, MOD 110 R/M, DISP-LO, DISP-HI]
                    printf("Error: Unimplemented DIV instruction: 0x%02X\n", memory[0]);
                    REGS->invalid_operations ++;
                    break;
                case 7: // IDIV REG8/MEM8: [0xF6, MOD 111 R/M, DISP-LO, DISP-HI]
                    printf("Error: Unimplemented IDIV instruction: 0x%02X\n", memory[0]);
                    REGS->invalid_operations ++;
                    break;
            }
            break;
        case 0xF7:
            switch(get_register_field(memory[1])) {
                case 0: // TEST REG16/MEM16, IMMED16
                    printf("Error: Invalid Instruction: 0x%02X 001\n", memory[0]);
                    REGS->invalid_operations ++;
                    break;
                case 1: // Not used
                    printf("Error: Invalid Instruction: 0x%02X 001\n", memory[0]);
                    REGS->invalid_operations ++;
                    break;
                case 2: // NOT REG16/MEM16
                    printf("Error: Invalid Instruction: 0x%02X 001\n", memory[0]);
                    REGS->invalid_operations ++;
                    break;
                case 3: // NEG REG16/MEM16
                    printf("Error: Invalid Instruction: 0x%02X 001\n", memory[0]);
                    REGS->invalid_operations ++;
                    break;
                case 4: // MUL REG16/MEM16
                    printf("Error: Invalid Instruction: 0x%02X 001\n", memory[0]);
                    REGS->invalid_operations ++;
                    break;
                case 5: // IMUL REG16/MEM16 (signed)
                    printf("Error: Invalid Instruction: 0x%02X 001\n", memory[0]);
                    REGS->invalid_operations ++;
                    break;
                case 6: // DIV REG16/MEM16
                    ret_val = div_instr(memory[0], &memory[1]);
                    break;
                case 7: // IDIV REG16/MEM16
                    printf("Error: Invalid Instruction: 0x%02X 001\n", memory[0]);
                    REGS->invalid_operations ++;
                    break;
            }
            break;
        case 0xF8:  // CLC (Clear Carry Flag): [0xF9]
            mylog("logs/main.log", "Instruction 0xF8: Clear Carry Flag (CF)\n");
            set_flag(CF, 0);
            break;
        case 0xF9:  // STC (Set Carry Flag): [0xF9]
            mylog("logs/main.log", "Instruction 0xF9: Set Carry Flag (CF)\n");
            set_flag(CF, 1);
            break;
        case 0xFA:  // CLI
            // Clear Interrupt Flag, causes the processor to ignore maskable external interrupts
            printf("Disabling interrupts\n");
            mylog("logs/main.log", "Instruction 0xFA: Clear Interrupt Flag (IF) to disable interrupts\n");
            set_flag(IF, 0);
            break;
        case 0xFB:  // STI
            // Set Interrupt Flag
            printf("Enabling interrupts\n");
            mylog("logs/main.log", "Instruction 0xFB: Set Interrupt Flag (IF) to enable interrupts\n");
            set_flag(IF, 1);
            break;
        case 0xFC:  // CLD: [0xFC]: Clear Direction Flag
            mylog("logs/main.log", "Instruction 0xFC: Clear Direction Flag (DF)\n");
            set_flag(DF, 0);
            break;
        case 0xFD:  // STD: [0xFD]: Set Direction Flag
            mylog("logs/main.log", "Instruction 0xFD: Set Direction Flag (DF)\n");
            set_flag(DF, 1);
            break;
        case 0xFE:
            ret_val = inc_dec_instr(memory[0], &memory[1]);
            break;
        case 0xFF:
            ret_val = inc_dec_instr(memory[0], &memory[1]);
            break;
        default:
            printf("Unknown instruction: 0x%02X\n", memory[0]);
            REGS->invalid_operations ++;
            // INVALID_INSTRUCTION;
    }
    processed_commands[memory[0]] += 1;
    return ret_val;
}

uint16_t delayed_int_timeout = 0;
uint16_t delayed_int_vector = 0xFFFF;

void set_delayed_int(uint8_t vector, uint16_t dely_ticks) {
    if(get_flag(IF)) {
        printf("Triggering interrupt %d\n", vector);
    } else {
        printf("Setting interrupt %d with IF = 0\n", vector);
    }
    delayed_int_vector = vector;
    delayed_int_timeout = dely_ticks;
}

void set_int_vector(uint8_t vector) {
    if(get_flag(IF)) {
        printf("Triggering interrupt %d\n", vector);
    } else {
        printf("Setting interrupt %d with IF = 0\n", vector);
    }
    REGS->int_vector = vector;
}

// Pront list of processed commands to log file
void print_proc_commands(void) {
    FILE *f;
    f = fopen(COMMON_LOG_FILE, "w");
    if(f == NULL) {
        printf("ERROR: Failed to open %s", COMMON_LOG_FILE);
        return;
    }
    fprintf(f,"List of processed commands:\n");
    for(int i=0; i<0xFF; i++) {
        fprintf(f,"Opcode 0x%02X: was executed %d times\n", i, processed_commands[i]);
    }
    fclose(f);
}

int store_registers(char *filename, registers_t *regs) {
    FILE *f;
    f = fopen(filename, "w");
    if(f == NULL) {
        printf("ERROR: Failed to open %s", filename);
        return EXIT_FAILURE;
    }
    fprintf(f, "0x%08X\n", regs->ticks);
    fprintf(f, "0x%04X\n", regs->AX);
    fprintf(f, "0x%04X\n", regs->BX);
    fprintf(f, "0x%04X\n", regs->CX);
    fprintf(f, "0x%04X\n", regs->DX);
    fprintf(f, "0x%04X\n", regs->SI);
    fprintf(f, "0x%04X\n", regs->DI);
    fprintf(f, "0x%04X\n", regs->BP);
    fprintf(f, "0x%04X\n", regs->SP);
    fprintf(f, "0x%04X\n", regs->IP);
    fprintf(f, "0x%04X\n", regs->CS);
    fprintf(f, "0x%04X\n", regs->DS);
    fprintf(f, "0x%04X\n", regs->ES);
    fprintf(f, "0x%04X\n", regs->SS);
    fprintf(f, "0x%04X\n", regs->flags);
    fprintf(f, "0x%04X\n", regs->prefixes);
    fclose(f);
    return EXIT_SUCCESS;
}

int restore_registers(char *filename, registers_t *regs) {
    FILE *f;
    f = fopen(filename, "r");
    if(f == NULL) {
        printf("ERROR: Failed to open %s", filename);
        return EXIT_FAILURE;
    }
    uint32_t temp;
    fscanf(f, "0x%08X\n", &regs->ticks);
    fscanf(f, "0x%04X\n", &temp);
    regs->AX = (uint16_t)temp;
    fscanf(f, "0x%04X\n", &temp);
    regs->BX = (uint16_t)temp;
    fscanf(f, "0x%04X\n", &temp);
    regs->CX = (uint16_t)temp;
    fscanf(f, "0x%04X\n", &temp);
    regs->DX = (uint16_t)temp;
    fscanf(f, "0x%04X\n", &temp);
    regs->SI = (uint16_t)temp;
    fscanf(f, "0x%04X\n", &temp);
    regs->DI = (uint16_t)temp;
    fscanf(f, "0x%04X\n", &temp);
    regs->BP = (uint16_t)temp;
    fscanf(f, "0x%04X\n", &temp);
    regs->SP = (uint16_t)temp;
    fscanf(f, "0x%04X\n", &temp);
    regs->IP = (uint16_t)temp;
    fscanf(f, "0x%04X\n", &temp);
    regs->CS = (uint16_t)temp;
    fscanf(f, "0x%04X\n", &temp);
    regs->DS = (uint16_t)temp;
    fscanf(f, "0x%04X\n", &temp);
    regs->ES = (uint16_t)temp;
    fscanf(f, "0x%04X\n", &temp);
    regs->SS = (uint16_t)temp;
    fscanf(f, "0x%04X\n", &temp);
    regs->flags = (uint16_t)temp;
    fscanf(f, "0x%04X\n", &temp);
    regs->prefixes = (uint16_t)temp;
    fclose(f);
    mylog("logs/main.log", "Registers restored! ticks = 0x%08X\n", regs->ticks);
    return EXIT_SUCCESS;
}

// static uint8_t *MEMORY = NULL;

int init_cpu(uint8_t continue_simulation) {
    // MEMORY = mem_init(continue_simulation);
    // if(MEMORY == NULL) {
    //     return EXIT_FAILURE;
    // }
    REGS = calloc(1, sizeof(registers_t));
    REGS->int_vector = 0xFFFF;
    if(continue_simulation) {
        mylog("logs/main.log", "Restoring registers\n");
        restore_registers(REGISTERS_FILE, REGS);
    } else {
        REGS->IP = 0xFFF0;
        REGS->CS = 0xF000;
    }
    return EXIT_SUCCESS;
}

void cpu_save_state(void) {
    store_registers(REGISTERS_FILE, REGS);
    // store_memory();
    // store_io();
}

uint8_t cpu_is_halted(void) {
    return (REGS->halt == 1) && (get_flag(IF) == 0);
}

// uint8_t ip_inc = 0;

int cpu_tick(void) {
    if(delayed_int_timeout > 0) {
        delayed_int_timeout --;
        if((delayed_int_timeout == 0) && (delayed_int_vector != 0xFFFF)) {
            set_int_vector(delayed_int_vector);
            delayed_int_vector = 0xFFFF;
        }
    }
    if(REGS->int_vector != 0xFFFF) {
        if(get_flag(IF)) {
            printf("CPU interrupt %d\n", REGS->int_vector);
            push_register(REGS->flags);
            push_register(REGS->CS);
            push_register(REGS->IP);
            set_register_value(IP_register, mem_read(4 * REGS->int_vector, 2));
            set_register_value(CS_register, mem_read((4 * REGS->int_vector) + 2, 2));
            REGS->int_vector = 0xFFFF;
            set_flag(IF, 0);
            set_flag(TF, 0);}
        }
    uint32_t addr = ((uint32_t)REGS->CS << 4) + REGS->IP;
    static uint8_t code[10];
    for(uint8_t i=0; i<sizeof(code); i++) {
        code[i] = mem_read(addr+i, 1);
    }
    uint8_t inc = process_instruction(code);
    if (cpu_is_halted()) {
        printf("ERROR: CPU is halted!\n");
        print_proc_commands();
        return EXIT_FAILURE;
    }
    if ((REGS->invalid_operations < 1)) {
        REGS->ticks++;
        REGS->IP += inc;
        // if(REGS->ticks & 0x0001) {
        //     timer_tick();
        // }
        return EXIT_SUCCESS;
    }
    // store_registers(REGISTERS_FILE, REGS);
    cpu_save_state();
    mylog("logs/main.log", "Processed commands: %d\n", REGS->ticks);
    print_proc_commands();
    return EXIT_FAILURE;
}

__declspec(dllexport)
/* Set space_type to 0 for IO_SPACE and to 1 for MEM_SPACE */
void connect_address_space(uint8_t space_type, WRITE_FUNC_PTR(write_func), READ_FUNC_PTR(read_func)) {
    if(space_type == 0) {   // IO_SPACE
        io_write = write_func;
        io_read = read_func;
    } else {   // MEM_SPACE
        mem_write = write_func;
        mem_read = read_func;
    }
}

__declspec(dllexport)
void set_code_read_func(READ_FUNC_PTR(read_func)) {
    code_read = read_func;
}

__declspec(dllexport)
void module_reset(void) {
    REGS = (registers_t*)calloc(1, sizeof(registers_t));
    REGS->int_vector = 0xFFFF;
    REGS->IP = 0xFFF0;
    REGS->CS = 0xF000;
    printf("REGS->IP = 0x%04X, REGS->CS = 0x%04X\n", REGS->IP, REGS->CS);
}

__declspec(dllexport)
void module_save(void) {
    store_data(REGS, sizeof(registers_t), CPU_DUMP_FILE);
}

__declspec(dllexport)
void module_restore(void) {
    registers_t *temp_regs = calloc(1, sizeof(registers_t));
    if(EXIT_SUCCESS == restore_data(temp_regs, sizeof(registers_t), CPU_DUMP_FILE)) {
        memcpy(REGS, temp_regs, sizeof(registers_t));
    }
}

__declspec(dllexport)
int module_tick(void) {
    // if(delayed_int_timeout > 0) {
    //     delayed_int_timeout --;
    //     if((delayed_int_timeout == 0) && (delayed_int_vector != 0xFFFF)) {
    //         set_int_vector(delayed_int_vector);
    //         delayed_int_vector = 0xFFFF;
    //     }
    // }
    if(REGS->int_vector != 0xFFFF) {
        if(get_flag(IF)) {
            printf("CPU interrupt %d\n", REGS->int_vector);
            push_register(REGS->flags);
            push_register(REGS->CS);
            push_register(REGS->IP);
            set_register_value(IP_register, mem_read(4 * REGS->int_vector, 2));
            set_register_value(CS_register, mem_read((4 * REGS->int_vector) + 2, 2));
            REGS->int_vector = 0xFFFF;
            set_flag(IF, 0);
            set_flag(TF, 0);}
        }
    uint32_t addr = ((uint32_t)REGS->CS << 4) + REGS->IP;
    static uint8_t code[10];
    for(uint8_t i=0; i<sizeof(code); i++) {
        code[i] = code_read(addr+i, 1);
    }
    uint8_t inc = process_instruction(code);
    if (cpu_is_halted()) {
        printf("ERROR: CPU is halted!\n");
        return EXIT_FAILURE;
    }
    if ((REGS->invalid_operations < 1)) {
        REGS->ticks++;
        REGS->IP += inc;
        if(REGS->IP == 0xF9A9) {
            printf("ERROR: E_MSG!\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

void dummy_nmi_cb(wire_state_t new_state) {
    if(new_state == WIRE_HIGH)
        mylog("logs/main.log", "NMI activated\n");
    return;
}

void int_cb(wire_state_t new_state) {
    if(new_state == WIRE_HIGH && REGS->ticks > 0) {
        mylog("logs/main.log", "INT activated\n");
        uint8_t vec = io_read(0xA0, 1);
        if(vec != 0xFF) {
            mylog("logs/short.log", "INT activated\n");
            set_int_vector(vec);
            // printf("CPU Interrupt %d\n", vec);
        }
    }
    return;
}

__declspec(dllexport)
wire_t nmi_wire = WIRE_T(WIRE_INPUT, &dummy_nmi_cb);

__declspec(dllexport)
wire_t int_wire = WIRE_T(WIRE_INPUT, &int_cb);
