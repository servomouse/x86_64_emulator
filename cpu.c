// http://ref.x86asm.net/coder64.html
#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>

uint8_t get_reg_opcode(uint8_t byte)
{
    byte >>= 3;
    byte &= 0x07;
    return byte;
}

void invalid_command()
{
    while(1);   // stop execution
}

void process_instruction(uint8_t * pc, uint16_t prefix, uint8_t second_byte)
{
    switch(pc[0])
    {
        case 0x00:  // ADD 8bit OP_0 is R/M, OP_1 is R
        break;
        case 0x01:  // ADD 16/32bit OP_0 is R/M, OP_1 is R
        break;
        case 0x02:  // ADD 8bit OP_0 is R, OP_1 is R/M
        break;
        case 0x03:  // ADD 16/32bit OP_0 is R, OP_1 is R/M
        break;
        case 0x04:  // ADD 8bit OP_0 is AL, OP_1 is imm8
        break;
        case 0x05:  // ADD 16/32bit OP_0 is EAX, OP_1 is imm16/32
        break;
        case 0x06:  // PUSH OP_0 is ES
        break;
        case 0x07:  // POP OP_0 is ES
        break;
        case 0x08:  // OR 8bit OP_0 is R/M, OP_1 is R
        break;
        case 0x09:  // OR 16/32bit OP_0 is R/M, OP_1 is R
        break;
        case 0x0A:  // OR 8bit OP_0 is R, OP_1 is R/M
        break;
        case 0x0B:  // OR 16/32bit OP_0 is R, OP_1 is R/M
        break;
        case 0x0C:  // OR 8bit OP_0 is AL, OP_1 is imm8
        break;
        case 0x0D:  // OR 16/32bit OP_0 is EAX, OP_1 is imm16/32
        break;
        case 0x0E:  // PUSH OP_0 is CS
        break;
        case 0x0F:  // Two-bytes long instruction
            process_instruction(pc+1, prefix, 1);
            break;
        case 0x10:  // ADC 8bit OP_0 is R/M, OP_1 is R
        break;
        case 0x11:  // ADC 16/32bit OP_0 is R/M, OP_1 is R
        break;
        case 0x12:  // ADC 8bit OP_0 is R, OP_1 is R/M
        break;
        case 0x13:  // ADC 16/32bit OP_0 is R, OP_1 is R/M
        break;
        case 0x14:  // ADC 8bit OP_0 is AL, OP_1 is imm8
        break;
        case 0x15:  // ADC 16/32bit OP_0 is EAX, OP_1 is imm16/32
        break;
        case 0x16:  // PUSH OP_0 is SS
        break;
        case 0x17:  // POP OP_0 is SS
        break;
        case 0x18:  // SBB 8bit OP_0 is R/M, OP_1 is R
        break;
        case 0x19:  // SBB 16/32bit OP_0 is R/M, OP_1 is R
        break;
        case 0x1A:  // SBB 8bit OP_0 is R, OP_1 is R/M
        break;
        case 0x1B:  // SBB 16/32bit OP_0 is R, OP_1 is R/M
        break;
        case 0x1C:  // SBB 8bit OP_0 is AL, OP_1 is imm8
        break;
        case 0x1D:  // SBB 16/32bit OP_0 is EAX, OP_1 is imm16/32
        break;
        case 0x1E:  // PUSH OP_0 is DS
        break;
        case 0x1F:  // POP OP_0 is DS
        break;
        case 0x20:  // AND 8bit OP_0 is R/M, OP_1 is R
        break;
        case 0x21:  // AND 16/32bit OP_0 is R/M, OP_1 is R
        break;
        case 0x22:  // AND 8bit OP_0 is R, OP_1 is R/M
        break;
        case 0x23:  // AND 16/32bit OP_0 is R, OP_1 is R/M
        break;
        case 0x24:  // AND 8bit OP_0 is AL, OP_1 is imm8
        break;
        case 0x25:  // AND 16/32bit OP_0 is EAX, OP_1 is imm16/32
        break;
        case 0x26:  // Segment override prefix ES
            prefix |= X26;
            process_instruction(pc+1, prefix, second_byte);
        break;
        case 0x27:  // DAA OP_0 is AL
        break;
        case 0x28:  // SUB 8bit OP_0 is R/M, OP_1 is R
        break;
        case 0x29:  // SUB 16/32bit OP_0 is R/M, OP_1 is R
        break;
        case 0x2A:  // SUB 8bit OP_0 is R, OP_1 is R/M
        break;
        case 0x2B:  // SUB 16/32bit OP_0 is R, OP_1 is R/M
        break;
        case 0x2C:  // SUB 8bit OP_0 is AL, OP_1 is imm8
        break;
        case 0x2D:  // SUB 16/32bit OP_0 is EAX, OP_1 is imm16/32
        break;
        case 0x2E:  // Segment override prefix CS
            prefix |= X2E;
            process_instruction(pc+1, prefix, second_byte);
        break;
        case 0x2F:  // DAS OP_0 is AL
        break;
        case 0x30:  // XOR 8bit OP_0 is R/M, OP_1 is R
        break;
        case 0x31:  // XOR 16/32bit OP_0 is R/M, OP_1 is R
        break;
        case 0x32:  // XOR 8bit OP_0 is R, OP_1 is R/M
        break;
        case 0x33:  // XOR 16/32bit OP_0 is R, OP_1 is R/M
        break;
        case 0x34:  // XOR 8bit OP_0 is AL, OP_1 is imm8
        break;
        case 0x35:  // XOR 16/32bit OP_0 is EAX, OP_1 is imm16/32
        break;
        case 0x36:  // Segment override prefix SS
            prefix |= X36;
            process_instruction(pc+1, prefix, second_byte);
        break;
        case 0x37:  // AAA 8bit OP_0 is AL, OP_1 is AH
        break;
        case 0x38:  // CMP 8bit OP_0 is R/M, OP_1 is R
        break;
        case 0x39:  // CMP 16/32bit OP_0 is R/M, OP_1 is R
        break;
        case 0x3A:  // CMP 8bit OP_0 is R, OP_1 is R/M
        break;
        case 0x3B:  // CMP 16/32bit OP_0 is R, OP_1 is R/M
        break;
        case 0x3C:  // CMP 8bit OP_0 is AL, OP_1 is imm8
        break;
        case 0x3D:  // CMP 16/32bit OP_0 is EAX, OP_1 is imm16/32
        break;
        case 0x3E:  // Segment override prefix DS
            prefix |= X3E;
            process_instruction(pc+1, prefix, second_byte);
        break;
        case 0x3F:  // AAS 8bit OP_0 is AL, OP_1 is AH
        break;
        case 0x40:  // INC Al/AX/EAX OP_0 is R16/32
        break;
        case 0x41:  // INC Cl/CX/ECX OP_0 is R16/32
        break;
        case 0x42:  // INC Dl/DX/EDX OP_0 is R16/32
        break;
        case 0x43:  // INC Bl/BX/EBX OP_0 is R16/32
        break;
        case 0x44:  // INC Ah/SP/ESP OP_0 is R16/32
        break;
        case 0x45:  // INC Ch/BP/EBP OP_0 is R16/32
        break;
        case 0x46:  // INC Dh/SI/ESI OP_0 is R16/32
        break;
        case 0x47:  // INC Bh/DI/EDI OP_0 is R16/32
        break;
        case 0x48:  // DEC Al/AX/EAX OP_0 is R16/32
        break;
        case 0x49:  // DEC Cl/CX/ECX OP_0 is R16/32
        break;
        case 0x4A:  // DEC Dl/DX/EDX OP_0 is R16/32
        break;
        case 0x4B:  // DEC Bl/BX/EBX OP_0 is R16/32
        break;
        case 0x4C:  // DEC Ah/SP/ESP OP_0 is R16/32
        break;
        case 0x4D:  // DEC Ch/BP/EBP OP_0 is R16/32
        break;
        case 0x4E:  // DEC Dh/SI/ESI OP_0 is R16/32
        break;
        case 0x4F:  // DEC Bh/DI/EDI OP_0 is R16/32
        break;
        case 0x50:  // PUSH Al/AX/EAX OP_0 is R16/32
        break;
        case 0x51:  // PUSH Cl/CX/ECX OP_0 is R16/32
        break;
        case 0x52:  // PUSH Dl/DX/EDX OP_0 is R16/32
        break;
        case 0x53:  // PUSH Bl/BX/EBX OP_0 is R16/32
        break;
        case 0x54:  // PUSH Ah/SP/ESP OP_0 is R16/32
        break;
        case 0x55:  // PUSH Ch/BP/EBP OP_0 is R16/32
        break;
        case 0x56:  // PUSH Dh/SI/ESI OP_0 is R16/32
        break;
        case 0x57:  // PUSH Bh/DI/EDI OP_0 is R16/32
        break;
        case 0x58:  // POP Al/AX/EAX OP_0 is R16/32
        break;
        case 0x59:  // POP Cl/CX/ECX OP_0 is R16/32
        break;
        case 0x5A:  // POP Dl/DX/EDX OP_0 is R16/32
        break;
        case 0x5B:  // POP Bl/BX/EBX OP_0 is R16/32
        break;
        case 0x5C:  // POP Ah/SP/ESP OP_0 is R16/32
        break;
        case 0x5D:  // POP Ch/BP/EBP OP_0 is R16/32
        break;
        case 0x5E:  // POP Dh/SI/ESI OP_0 is R16/32
        break;
        case 0x5F:  // POP Bh/DI/EDI OP_0 is R16/32
        break;
        case 0x60:  // PUSHA/PUSHAD: AX, CX, DX, BX, SP, BP, SI, DI / EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
        break;
        case 0x61:  // POPA/POPAD: DI, SI, BP, BX, DX, CX, AX / EDI, ESI, EBP, EBX, EDX, ECX, EAX
        break;
        case 0x62:  // BOUND 16/32bit OP_0 is R, OP_1 is R/M
        break;
        case 0x63:  // ARPL 16 bit OP_0 is R/M, OP_1 is R
        break;
        case 0x64:  // Segment override prefix FS
            prefix |= X64;
            process_instruction(pc+1, prefix, second_byte);
        break;
        case 0x65:  // Segment override prefix GS
            prefix |= X65;
            process_instruction(pc+1, prefix, second_byte);
        break;
        case 0x66:  // Operand override prefix
            prefix |= X66;
            process_instruction(pc+1, prefix, second_byte);
        break;
        case 0x67:  // Address override prefix
            prefix |= X67;
            process_instruction(pc+1, prefix, second_byte);
        break;
        case 0x68:  // PUSH OP_0 is imm16/32
        break;
        case 0x69:  // IMUL 16/32bit OP_0 is R, OP_0 is R/M, OP_0 is imm16/32
        break;
        case 0x6A:  // PUSH OP_0 is imm8
        break;
        case 0x6B:  // IMUL 16/32bit OP_0 is R, OP_0 is R/M, OP_0 is imm8
        break;
        case 0x6C:  // INS/INSB 8bit OP_0 is M, OP_1 is DX
        break;
        case 0x6D:  // INS/INSW 16bit OP_0 is M, OP_1 is DX
        break;
        case 0x6E:  // OUTS/OUTSB 8bit OP_0 is DX, OP_1 is M
        break;
        case 0x6F:  // OUTS/OUTSW 16bit OP_0 is DX, OP_1 is M
        break;
        case 0x70:  // JO OP_0 is REL8
        break;
        case 0x71:  // JNO OP_0 is REL8
        break;
        case 0x72:  // JB/JNAE/JC OP_0 is REL8
        break;
        case 0x73:  // JNB/JAE/JNC OP_0 is REL8
        break;
        case 0x74:  // JZ/JE OP_0 is REL8
        break;
        case 0x75:  // JNZ/JNE OP_0 is REL8
        break;
        case 0x76:  // JBE/JNA OP_0 is REL8
        break;
        case 0x77:  // JNBE/JA OP_0 is REL8
        break;
        case 0x78:  // JS OP_0 is REL8
        break;
        case 0x79:  // JNS OP_0 is REL8
        break;
        case 0x7A:  // JP OP_0 is REL8
        break;
        case 0x7B:  // JPE OP_0 is REL8
        break;
        case 0x7C:  // JL/JNGE OP_0 is REL8
        break;
        case 0x7D:  // JNL/JGE OP_0 is REL8
        break;
        case 0x7E:  // JLE/JNG OP_0 is REL8
        break;
        case 0x7F:  // JNLE/JG OP_0 is REL8
        break;
        case 0x80:
        if(second_byte == 0)
        {
            switch(get_reg_opcode(pc[1]))
            {
                case 0: // ADD 8bit OP_0 is R/M, OP_1 is imm8
                break;
                case 1: // OR 8bit OP_0 is R/M, OP_1 is imm8
                break;
                case 2: // ADC 8bit OP_0 is R/M, OP_1 is imm8
                break;
                case 3: // SBB 8bit OP_0 is R/M, OP_1 is imm8
                break;
                case 4: // AND 8bit OP_0 is R/M, OP_1 is imm8
                break;
                case 5: // SUB 8bit OP_0 is R/M, OP_1 is imm8
                break;
                case 6: // XOR 8bit OP_0 is R/M, OP_1 is imm8
                break;
                case 7: // CMP 8bit OP_0 is R/M, OP_1 is imm8
                break;
            }
        }
        break;
        case 0x81:
        if(second_byte == 0)
        {
            switch(get_reg_opcode(pc[1]))
            {
                case 0: // ADD 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
                case 1: // OR 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
                case 2: // ADC 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
                case 3: // SBB 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
                case 4: // AND 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
                case 5: // SUB 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
                case 6: // XOR 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
                case 7: // CMP 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
            }
        }
        break;
        case 0x82:
        if(second_byte == 0)
        {
            switch(get_reg_opcode(pc[1]))
            {
                case 0: // ADD 8bit OP_0 is R/M, OP_1 is imm8
                break;
                case 1: // OR 8bit OP_0 is R/M, OP_1 is imm8
                break;
                case 2: // ADC 8bit OP_0 is R/M, OP_1 is imm8
                break;
                case 3: // SBB 8bit OP_0 is R/M, OP_1 is imm8
                break;
                case 4: // AND 8bit OP_0 is R/M, OP_1 is imm8
                break;
                case 5: // SUB 8bit OP_0 is R/M, OP_1 is imm8
                break;
                case 6: // XOR 8bit OP_0 is R/M, OP_1 is imm8
                break;
                case 7: // CMP 8bit OP_0 is R/M, OP_1 is imm8
                break;
            }
        }
        break;
        case 0x83:
        if(second_byte == 0)
        {
            switch(get_reg_opcode(pc[1]))
            {
                case 0: // ADD 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
                case 1: // OR 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
                case 2: // ADC 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
                case 3: // SBB 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
                case 4: // AND 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
                case 5: // SUB 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
                case 6: // XOR 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
                case 7: // CMP 16/32bit OP_0 is R/M, OP_1 is imm16/32
                break;
            }
        }
        break;
        case 0x84: // TEST 8bit OP_0 is R/M, OP_1 is R  (Logical Compare)
        break;
        case 0x85: // TEST 16/32bit OP_0 is R/M, OP_1 is R  (Logical Compare)
        break;
        case 0x86: // XCHG 8bit OP_0 is R/M, OP_1 is R (Exchange Register/Memory with Register)
        break;
        case 0x87: // XCHG 16/32bit OP_0 is R/M, OP_1 is R  (Exchange Register/Memory with Register)
        break;
        case 0x88:  // MOVE 8bit OP_0 is R/M, OP_1 is R
        break;
        case 0x89:  // MOVE 16/32bit OP_0 is R/M, OP_1 is R
        break;
        case 0x8A:  // MOVE 8bit OP_0 is R, OP_1 is R/M
        break;
        case 0x8B:  // MOVE 16/32bit OP_0 is R, OP_1 is R/M
        break;
        case 0x8C:  // MOVE 16/32bit OP_0 is R/M, OP_1 is Sreg
        break;
        case 0x8D:  // LEA 16/32bit OP_0 is R, OP_1 is M  (Load Effective Address)
        break;
        case 0x8E:  // MOVE 16/32bit OP_0 is Sreg, OP_1 is R/M
        break;
        case 0x8F:  // POP 16/32bit OP_0 is R/M (Pop a Value from the Stack)
        break;
        case 0x90:
        if(second_byte == 0)
        {
            if(prefix && XF3)   // PAUSE
                ;
            else    // NOP
                ;
        }
        break;
        case 0x91: // XCHG 16/32bit OP_0 is ECX, OP_1 is EAX  (Exchange regsiters value)
        break;
        case 0x92: // XCHG 16/32bit OP_0 is EDX, OP_1 is EAX  (Exchange regsiters value)
        break;
        case 0x93: // XCHG 16/32bit OP_0 is EBX, OP_1 is EAX  (Exchange regsiters value)
        break;
        case 0x94: // XCHG 16/32bit OP_0 is ESP, OP_1 is EAX  (Exchange regsiters value)
        break;
        case 0x95: // XCHG 16/32bit OP_0 is EBP, OP_1 is EAX  (Exchange regsiters value)
        break;
        case 0x96: // XCHG 16/32bit OP_0 is ESI, OP_1 is EAX  (Exchange regsiters value)
        break;
        case 0x97: // XCHG 16/32bit OP_0 is EDI, OP_1 is EAX  (Exchange regsiters value)
        break;
        case 0x98:  // CBW/CBWE OP_0 is AX/EAX, OP_1 is AL/AX (convert byte/word to word/doubleword)
        break;
        case 0x99:  // CWD/CDQ OP_0 is DX/EDX, OP_1 is AX/EAX (convert word/doubleword to dpubleword/quadword)
        break;
        case 0x9A:  // CALLF OP_0 is ptr16:16/32 (call procedure)
        break;
        case 0x9B:  // WAIT prefix or FWAIT (Check pending unmasked floating-point exceptions)
            prefix |= X9B;
            process_instruction(pc+1, prefix, second_byte);
        break;
        case 0x9C:  // PUSHF/PUSHFD, OP_0 is FLAGS/eFLAGS register (push FLAGS/eFLAGS register onto stack)
        break;
        case 0x9D:  // POPF/POPFD, OP_0 is FLAGS/eFLAGS register (pop stack into FLAGS/eFLAGS register)
        break;
        case 0x9E:  // SAHF, OP_0 is AH register (store AH into FLAGS register)
        break;
        case 0x9F:  // LAHF, op_0 is AH register (load status FLAGS into AH register)
        break;
        case 0xA0:  // MOV 8bit, OP_0 is AL, OP_1 is moffs8
        break;
        case 0xA1:  // MOV 16/32bit, OP_0 is EAX, OP_1 is moffs16/32
        break;
        case 0xA2:  // MOV 8bit, OP_0 is moffs8, OP_1 is AL
        break;
        case 0xA3:  // MOV 16/32bit, OP_0 is moffs16/32, OP_1 is EAX
        break;
        case 0xA4:  // MOVS/MOVSB, OP_0 is m8, OP_1 is m8 (move string to string)
        break;
        case 0xA5:  // MOVS/MOVSW/MOVSD OP_0 is m16/32, OP_1 is m16/32 (move string to string)
        break;
        case 0xA6:  // CMPS/CMPSB, OP_0 is m8, OP_1 is m8 (compare strings)
        break;
        case 0xA7:  // CMPS/CMPSW/CMPSD OP_0 is m16/32, OP_1 is m16/32 (compare strings)
        break;
        case 0xA8:  // TEST, OP_0 is AL, OP_1 is immm8 (logical compare)
        break;
        case 0xA9:  // TEST, OP_0 is EAX, OP_1 is immm16/32 (logical compare)
        break;
        case 0xAA:  // STOS/STOSB, OP_0 is m8, OP_1 is AL (store string) 
        break;
        case 0xAB:  // STOS/STOSW/STOSD, [OP_0 is m16, OP_1 is AX]/[OP_0 is m16/32, OP_1 is EAX] (store string)
        break;
        case 0xAC:  // LODS/LODSB, OP_0 is AL, OP_1 is m8 (load string)
        break;
        case 0xAD:  // LODS/LODSW/LODSD, [OP_0 is AX, OP_1 is m16]/[OP_0 is EAX, OP_1 is m16/32] (load string)
        break;
        case 0xAE:  // SCAS/SCASB, OP_0 is m8, OP_1 is AL (scan string)
        break;
        case 0xAF:  // SCAS/SCASW/SCASD, [OP_0 is m16, OP_1 is AX]/[OP_0 is m16/32, OP_1 is EAX] (scan string)
        break;
        case 0xB0:  // MOV 8bit, OP_0 is AL, OP_1 is m8
        break;
        case 0xB1:  // MOV 8bit, OP_0 is CL, OP_1 is m8
        break;
        case 0xB2:  // MOV 8bit, OP_0 is DL, OP_1 is m8
        break;
        case 0xB3:  // MOV 8bit, OP_0 is BL, OP_1 is m8
        break;
        case 0xB4:  // MOV 8bit, OP_0 is AH, OP_1 is m8
        break;
        case 0xB5:  // MOV 8bit, OP_0 is CH, OP_1 is m8
        break;
        case 0xB6:  // MOV 8bit, OP_0 is DH, OP_1 is m8
        break;
        case 0xB7:  // MOV 8bit, OP_0 is BH, OP_1 is m8
        break;
        case 0xB8:  // MOV 16/32bit, OP_0 is EAX, OP_1 is m16/32
        break;
        case 0xB9:  // MOV 16/32bit, OP_0 is ECX, OP_1 is m16/32
        break;
        case 0xBA:  // MOV 16/32bit, OP_0 is EDX, OP_1 is m16/32
        break;
        case 0xBB:  // MOV 16/32bit, OP_0 is EBX, OP_1 is m16/32
        break;
        case 0xBC:  // MOV 16/32bit, OP_0 is ESP, OP_1 is m16/32
        break;
        case 0xBD:  // MOV 16/32bit, OP_0 is EBP, OP_1 is m16/32
        break;
        case 0xBE:  // MOV 16/32bit, OP_0 is ESI, OP_1 is m16/32
        break;
        case 0xBF:  // MOV 16/32bit, OP_0 is EDI, OP_1 is m16/32
        break;
        case 0xC0:
            switch(get_reg_opcode(pc[1]))
            {
                case 0: // ROL 8bit OP_0 is R/M, OP_1 is imm8   (rotate)
                break;
                case 1: // ROR 8bit OP_0 is R/M, OP_1 is imm8   (rotate)
                break;
                case 2: // RCL 8bit OP_0 is R/M, OP_1 is imm8   (rotate)
                break;
                case 3: // RCR 8bit OP_0 is R/M, OP_1 is imm8   (rotate)
                break;
                case 4: // SHL/SAL 8bit OP_0 is R/M, OP_1 is imm8   (shift)
                break;
                case 5: // SHR 8bit OP_0 is R/M, OP_1 is imm8   (shift)
                break;
                case 6: // SAL/SHL 8bit OP_0 is R/M, OP_1 is imm8   (shift)
                break;
                case 7: // SAR 8bit OP_0 is R/M, OP_1 is imm8   (shift)
                break;
            }
        break;
        case 0xC1:
            switch(get_reg_opcode(pc[1]))
            {
                case 0: // ROL 16/32/64 bit OP_0 is R/M, OP_1 is imm8   (rotate)
                break;
                case 1: // ROR 16/32/64 bit OP_0 is R/M, OP_1 is imm8   (rotate)
                break;
                case 2: // RCL 16/32/64 bit OP_0 is R/M, OP_1 is imm8   (rotate)
                break;
                case 3: // RCR 16/32/64 bit OP_0 is R/M, OP_1 is imm8   (rotate)
                break;
                case 4: // SHL/SAL 16/32/64 bit OP_0 is R/M, OP_1 is imm8   (shift)
                break;
                case 5: // SHR 16/32/64 bit OP_0 is R/M, OP_1 is imm8   (shift)
                break;
                case 6: // SAL/SHL 16/32/64 bit OP_0 is R/M, OP_1 is imm8   (shift)
                break;
                case 7: // SAR 16/32/64 bit OP_0 is R/M, OP_1 is imm8   (shift)
                break;
            }
        break;
        case 0xC2:  // RETN, OP_0 is imm16  (return from procedure)
        break;
        case 0xC3:  // RETN  (return from procedure)
        break;
        case 0xC4:
            invalid_command();
        break;
        case 0xC5:
            invalid_command();
        break;
        case 0xC6:
            switch(get_reg_opcode(pc[1]))
            {
                case 0: // MOV 8bit, OP_0 is R/M, OP_1 is imm8  (move)
                break;
                default:
                invalid_command();
                break;
            }
        break;
        case 0xC7:
            switch(get_reg_opcode(pc[1]))
            {
                case 0: // MOV 16/32/64 bit, OP_0 is R/M, OP_1 is imm16/32  (move)
                break;
                default:
                invalid_command();
                break;
            }
        break;
        case 0xC8:  // ENTER OP_0 is rBP, OP_1 is imm16, OP_2 is imm8 (make stack frame for procedure parameters)
        break;
        case 0xC9:  // LEAVE OP_0 is rBP (high level procedure exit)
        break;
        case 0xCA:  // RETF OP_0 is imm16   (return from procedure)
        break;
        case 0xCB:  // RETF (return from procedure)
        break;
        case 0xCC:  // INT OP_0 is 3, OP_1 is eFlags (call to interrupt procedure)
        break;
        case 0xCD:  // INT OP_0 is imm8, OP_1 is eFlags (call to interrupt procedure)
        break;
        case 0xCE:  // INT0 OP_0 is eFlags (call to interrupt procedure)
        break;
        case 0xCF:  // IRET/IRETD/IRETQ OP_0 is Flags/EFlags/RFlags (interrupt return)
        break;
        case 0xD0:
            switch(get_reg_opcode(pc[1]))
            {
                case 0: // ROL 8 bit OP_0 is R/M, OP_1 is 1   (rotate)
                break;
                case 1: // ROR 8 bit OP_0 is R/M, OP_1 is 1   (rotate)
                break;
                case 2: // RCL 8 bit OP_0 is R/M, OP_1 is 1   (rotate)
                break;
                case 3: // RCR 8 bit OP_0 is R/M, OP_1 is 1   (rotate)
                break;
                case 4: // SHL/SAL 8 bit OP_0 is R/M, OP_1 is 1   (shift)
                break;
                case 5: // SHR 8 bit OP_0 is R/M, OP_1 is 1   (shift)
                break;
                case 6: // SAL/SHL 8 bit OP_0 is R/M, OP_1 is 1   (shift)
                break;
                case 7: // SAR 8 bit OP_0 is R/M, OP_1 is 1   (shift)
                break;
            }
        break;
        case 0xD1:
            switch(get_reg_opcode(pc[1]))
            {
                case 0: // ROL 16/32/64 bit OP_0 is R/M, OP_1 is 1   (rotate)
                break;
                case 1: // ROR 16/32/64 bit OP_0 is R/M, OP_1 is 1   (rotate)
                break;
                case 2: // RCL 16/32/64 bit OP_0 is R/M, OP_1 is 1   (rotate)
                break;
                case 3: // RCR 16/32/64 bit OP_0 is R/M, OP_1 is 1   (rotate)
                break;
                case 4: // SHL/SAL 16/32/64 bit OP_0 is R/M, OP_1 is 1   (shift)
                break;
                case 5: // SHR 16/32/64 bit OP_0 is R/M, OP_1 is 1   (shift)
                break;
                case 6: // SAL/SHL 16/32/64 bit OP_0 is R/M, OP_1 is 1   (shift)
                break;
                case 7: // SAR 16/32/64 bit OP_0 is R/M, OP_1 is 1   (shift)
                break;
            }
        break;
        case 0xD2:
            switch(get_reg_opcode(pc[1]))
            {
                case 0: // ROL 8 bit OP_0 is R/M, OP_1 is CL   (rotate)
                break;
                case 1: // ROR 8 bit OP_0 is R/M, OP_1 is CL   (rotate)
                break;
                case 2: // RCL 8 bit OP_0 is R/M, OP_1 is CL   (rotate)
                break;
                case 3: // RCR 8 bit OP_0 is R/M, OP_1 is CL   (rotate)
                break;
                case 4: // SHL/SAL 8 bit OP_0 is R/M, OP_1 is CL   (shift)
                break;
                case 5: // SHR 8 bit OP_0 is R/M, OP_1 is CL   (shift)
                break;
                case 6: // SAL/SHL 8 bit OP_0 is R/M, OP_1 is CL   (shift)
                break;
                case 7: // SAR 8 bit OP_0 is R/M, OP_1 is CL   (shift)
                break;
            }
        break;
        case 0xD3:
            switch(get_reg_opcode(pc[1]))
            {
                case 0: // ROL 16/32/64 bit OP_0 is R/M, OP_1 is CL   (rotate)
                break;
                case 1: // ROR 16/32/64 bit OP_0 is R/M, OP_1 is CL   (rotate)
                break;
                case 2: // RCL 16/32/64 bit OP_0 is R/M, OP_1 is CL   (rotate)
                break;
                case 3: // RCR 16/32/64 bit OP_0 is R/M, OP_1 is CL   (rotate)
                break;
                case 4: // SHL/SAL 16/32/64 bit OP_0 is R/M, OP_1 is CL   (shift)
                break;
                case 5: // SHR 16/32/64 bit OP_0 is R/M, OP_1 is CL   (shift)
                break;
                case 6: // SAL/SHL 16/32/64 bit OP_0 is R/M, OP_1 is CL   (shift)
                break;
                case 7: // SAR 16/32/64 bit OP_0 is R/M, OP_1 is CL   (shift)
                break;
            }
        break;
        case 0xD4:
            invalid_command();
        break;
        case 0xD5:
            invalid_command();
        break;
        case 0xD6:
            invalid_command();
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
        case 0xF0:  // LOCK prefix
            prefix |= XF0;
            process_instruction(pc+1, prefix, second_byte);
            break;
        case 0xF1:
        break;
        case 0xF2:  // REPNE prefix
            prefix |= XF2;
            process_instruction(pc+1, prefix, second_byte);
        break;
        case 0xF3:  // REP, REPE prefix
            prefix |= XF3;
            process_instruction(pc+1, prefix, second_byte);
        break;
        case 0xF4:
        break;
        case 0xF5:
        break;
        case 0xF6:
        break;
        case 0xF7:
        break;
        case 0xF8:  // CLC (Clear Carry Flag)
        break;
        case 0xF9:  // STC (Set Carry Flag)
        break;
        case 0xFA:  // CLI  (Clear Interrupt Flag)
        break;
        case 0xFB:  // STI (Set Interrupt Flag)
        break;
        case 0xFC:  // CLD (Clear Direction Flag)
        break;
        case 0xFD:  // STD (Set Direction Flag)
        break;
        case 0xFE:
        if(second_byte == 0)
        {
            switch(get_reg_opcode(pc[1]))
            {
                case 0: // INC 8bit OP_0 is R/M
                break;
                case 1: // DEC 8bit OP_0 is R/M
                break;
                default:
                invalid_command();
            }
        }
        break;
        case 0xFF:
        if(second_byte == 0)
        {
            switch(get_reg_opcode(pc[1]))
            {
                case 0: // INC 16/32bit OP_0 is R/M
                break;
                case 1: // DEC 16/32bit OP_0 is R/M
                break;
                case 2: // CALL 16/32bit OP_0 is R/M
                break;
                case 3: // CALLF  OP_0 is m16:16/32bit
                break;
                case 4: // JMP 16/32bit OP_0 is R/M
                break;
                case 5: // JMPF  OP_0 is m16:16/32bit
                break;
                case 6: // PUSH 16/32bit OP_0 is R/M (Push Word, Doubleword or Quadword Onto the Stack)
                break;
                case 7: // Invalid
                break;
            }
        }
        break;
    }
}

typedef enum {
    X26 = 0x01,
    X2E = 0x02,
    X36 = 0x04,
    X3E = 0x08,
    X64 = 0x10,
    X65 = 0x20,
    X66 = 0x40,
    X67 = 0x80,
    XF0 = 0x100,
    XF2 = 0x200,
    XF3 = 0x400,
    X9B = 0x800
} prefix_t;

// Prefixes:
// 0x26 = Segment override prefix ES
// 0x2E = Segment override prefix CS
// 0x36 = Segment override prefix SS
// 0x3E = Segment override prefix DS
// 0x64 = Segment override prefix FS
// 0x65 = Segment override prefix GS
// 0x66 = Operand override
// 0x67 = Address override
// 0x9B = wait prefix
// 0xF0 = LOCK
// 0xF2 = REPNE
// 0xF3 = REP, REPE

uint8_t code[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

int main(void)
{
    process_instruction(code, 0, 0);
    return 0;
}
