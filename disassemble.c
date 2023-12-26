#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./headers/disassemble.h"
bool is_valid_reg(y86_t *cpu, memory_t memory, y86_inst_t *ins);
bool validate_pc(y86_t *cpu, int offset);
y86_inst_t set_invalid_ins(y86_t *cpu, y86_inst_t *ins);
void print_spacing(int printed);
//============================================================================
/*
 * Print the usage message for this program.
 */
void usage_dis()
{
    printf("Usage: y86 <option(s)> mini-elf-file\n");
    printf(" Options are:\n");
    printf("  -h      Display usage\n");
    printf("  -H      Show the Mini-ELF header\n");
    printf("  -a      Show all with brief memory\n");
    printf("  -f      Show all with full memory\n");
    printf("  -s      Show the program headers\n");
    printf("  -m      Show the memory contents (brief)\n");
    printf("  -M      Show the memory contents (full)\n");
    printf("  -d      Disassemble code contents\n");
    printf("  -D      Disassemble data contents\n");

    printf("Options must not be repeated neither explicitly nor implicitly.\n");
}
//============================================================================
/*
 * Read the header from the file and store it in the struct.
 * Return true if the header is valid, false otherwise.
 */
y86_inst_t fetch(y86_t *cpu, memory_t memory)
{
    y86_inst_t ins;

    // Initialize the instruction
    memset(&ins, 0, sizeof(y86_inst_t)); // Clear all fields on instr.
    ins.type = INVALID;                  // Invalid instruction until proven otherwise

    if (!cpu || !memory || cpu->pc < 0 || cpu->pc >= MEMSIZE)
    {
        cpu->stat = cpu && memory ? ADR : INS;

        return ins;
    }
    // Fetch the opcode byte
    uint8_t opcode = memory[cpu->pc];

    // Update the instruction's opcode field
    ins.opcode = opcode;

    switch (opcode)
    {
    case 0x00:
        ins.type = HALT;
        ins.size = 1;
        ins.opcode = opcode;
        cpu->stat = HLT;
        break;
    case 0x10:
        ins.type = NOP;
        ins.size = 1;
        ins.opcode = opcode;
        break;
    case 0x20:
        if (!validate_pc(cpu, 1))
        {
            cpu->stat = ADR;
            return set_invalid_ins(cpu, &ins);
        }
        ins.type = CMOV;
        ins.cmov = RRMOVQ;
        ins.size = 2;
        ins.opcode = opcode;
        cpu->stat = AOK;
        if (!is_valid_reg(cpu, memory, &ins))
        {
            cpu->stat = INS;
            ins.type = INVALID;
            return ins;
        }
        break;
    case 0x21:
        if (!validate_pc(cpu, 1))
        {
            cpu->stat = ADR;
            return set_invalid_ins(cpu, &ins);
        }
        ins.type = CMOV;
        ins.cmov = CMOVLE;
        ins.size = 2;
        ins.opcode = opcode;
        cpu->stat = AOK;
        if (!is_valid_reg(cpu, memory, &ins))
        {
            cpu->stat = INS;
            ins.type = INVALID;
            return ins;
        }
        break;
    case 0x22:
        if (!validate_pc(cpu, 1))
        {
            cpu->stat = ADR;
            return set_invalid_ins(cpu, &ins);
        }
        ins.type = CMOV;
        ins.cmov = CMOVL;
        ins.size = 2;
        ins.opcode = opcode;
        cpu->stat = AOK;
        if (!is_valid_reg(cpu, memory, &ins))
        {
            cpu->stat = INS;

            ins.type = INVALID;
            return ins;
        }
        break;
    case 0x23:
        if (!validate_pc(cpu, 1))
        {
            cpu->stat = ADR;
            return set_invalid_ins(cpu, &ins);
        }
        ins.type = CMOV;
        ins.cmov = CMOVE;
        ins.size = 2;
        ins.opcode = opcode;
        cpu->stat = AOK;
        if (!is_valid_reg(cpu, memory, &ins))
        {
            cpu->stat = INS;

            ins.type = INVALID;
            return ins;
        }
        break;
    case 0x24:
        if (!validate_pc(cpu, 1))
        {
            cpu->stat = ADR;
            return set_invalid_ins(cpu, &ins);
        }
        ins.type = CMOV;
        ins.cmov = CMOVNE;
        ins.size = 2;
        ins.opcode = opcode;
        cpu->stat = AOK;
        if (!is_valid_reg(cpu, memory, &ins))
        {
            cpu->stat = INS;

            ins.type = INVALID;
            return ins;
        }
        break;
    case 0x25:
        if (!validate_pc(cpu, 1))
        {
            cpu->stat = ADR;
            return set_invalid_ins(cpu, &ins);
        }
        ins.type = CMOV;
        ins.cmov = CMOVGE;
        ins.size = 2;
        ins.opcode = opcode;
        cpu->stat = AOK;
        if (!is_valid_reg(cpu, memory, &ins))
        {
            cpu->stat = INS;

            ins.type = INVALID;
            return ins;
        }
        break;
    case 0x26:
        if (!validate_pc(cpu, 1))
        {
            cpu->stat = ADR;
            return set_invalid_ins(cpu, &ins);
        }
        ins.type = CMOV;
        ins.cmov = CMOVG;
        ins.size = 2;
        ins.opcode = opcode;
        cpu->stat = AOK;
        if (!is_valid_reg(cpu, memory, &ins))
        {
            cpu->stat = INS;

            ins.type = INVALID;
            return ins;
        }
        break;
    case 0x30:
        ins.type = IRMOVQ;
        ins.size = 10;
        ins.opcode = opcode;
        ins.ra = memory[cpu->pc + 1] >> 4;
        ins.rb = memory[cpu->pc + 1] & 0x0F;
        if (ins.ra != 0xF || ins.rb < 0 || ins.rb > 14)
        {
            cpu->stat = INS;

            ins.type = INVALID;
            return ins;
        }
        ins.value = 0;
        memcpy(&ins.value, &memory[cpu->pc + 2], 8);
        break;
    case 0x40:
        if (!validate_pc(cpu, 9))
        {
            cpu->stat = ADR;
            return set_invalid_ins(cpu, &ins);
        }
        ins.type = RMMOVQ;
        ins.size = 10;
        ins.opcode = opcode;
        if (!is_valid_reg(cpu, memory, &ins))
        {
            cpu->stat = INS;
            ins.type = INVALID;
            return ins;
        }
        ins.dest = 0;
        memcpy(&ins.d, &memory[cpu->pc + 2], 8);

        break;
    case 0x50:
        ins.type = MRMOVQ;
        ins.size = 10;
        ins.opcode = opcode;
        if (!is_valid_reg(cpu, memory, &ins))
        {
            cpu->stat = INS;

            ins.type = INVALID;
            return ins;
        }
        ins.d = 0;
        memcpy(&ins.d, &memory[cpu->pc + 2], 8);
        break;
    case 0x60:
        ins.type = OPQ;
        ins.op = ADD;
        ins.size = 2;
        ins.opcode = opcode;
        if (!is_valid_reg(cpu, memory, &ins))
        {
            cpu->stat = INS;

            ins.type = INVALID;
            return ins;
        }
        break;
    case 0x61:
        ins.type = OPQ;
        ins.op = SUB;
        ins.size = 2;
        ins.opcode = opcode;
        if (!is_valid_reg(cpu, memory, &ins))
        {
            cpu->stat = INS;

            ins.type = INVALID;
            return ins;
        }
        break;
    case 0x62:
        ins.type = OPQ;
        ins.op = AND;
        ins.size = 2;
        ins.opcode = opcode;
        if (!is_valid_reg(cpu, memory, &ins))
        {
            cpu->stat = INS;

            ins.type = INVALID;
            return ins;
        }
        break;
    case 0x63:
        ins.type = OPQ;
        ins.op = XOR;
        ins.size = 2;
        ins.opcode = opcode;
        if (!is_valid_reg(cpu, memory, &ins))
        {
            cpu->stat = INS;

            ins.type = INVALID;
            return ins;
        }
        break;
    case 0x70:
        ins.type = JUMP;
        ins.jump = JMP;
        ins.size = 9;
        ins.opcode = opcode;
        ins.dest = 0;
        memcpy(&ins.dest, &memory[cpu->pc + 1], 8);
        break;
    case 0x71:
        ins.type = JUMP;
        ins.jump = JLE;
        ins.size = 9;
        ins.opcode = opcode;
        ins.dest = 0;
        memcpy(&ins.dest, &memory[cpu->pc + 1], 8);
        break;
    case 0x72:
        ins.type = JUMP;
        ins.jump = JL;
        ins.size = 9;
        ins.opcode = opcode;
        ins.dest = 0;
        memcpy(&ins.dest, &memory[cpu->pc + 1], 8);
        break;
    case 0x73:
        ins.type = JUMP;
        ins.jump = JE;
        ins.size = 9;
        ins.opcode = opcode;
        ins.dest = 0;
        memcpy(&ins.dest, &memory[cpu->pc + 1], 8);
        break;
    case 0x74:
        ins.type = JUMP;
        ins.jump = JNE;
        ins.size = 9;
        ins.opcode = opcode;
        ins.dest = 0;
        memcpy(&ins.dest, &memory[cpu->pc + 1], 8);
        break;
    case 0x75:
        ins.type = JUMP;
        ins.jump = JGE;
        ins.size = 9;
        ins.opcode = opcode;
        ins.dest = 0;
        memcpy(&ins.dest, &memory[cpu->pc + 1], 8);
        break;
    case 0x76:
        ins.type = JUMP;
        ins.jump = JG;
        ins.size = 9;
        ins.opcode = opcode;
        ins.dest = 0;
        memcpy(&ins.dest, &memory[cpu->pc + 1], 8);
        break;
    case 0x80:
        if (!validate_pc(cpu, 8))
        {
            cpu->stat = ADR;
            return set_invalid_ins(cpu, &ins);
        }
        ins.type = CALL;
        ins.size = 9;
        ins.opcode = opcode;
        ins.dest = 0;
        memcpy(&ins.dest, &memory[cpu->pc + 1], 8);
        break;
    case 0x90:
        ins.type = RET;
        ins.size = 1;
        ins.opcode = opcode;
        break;
    case 0xA0:
        ins.type = PUSHQ;
        ins.size = 2;
        ins.opcode = opcode;
        ins.ra = memory[cpu->pc + 1] >> 4;
        ins.rb = memory[cpu->pc + 1] & 0x0F;
        if (ins.rb != 0xf)
        {
            cpu->stat = INS;

            ins.type = INVALID;
            return ins;
        }
        break;
    case 0xB0:
        ins.type = POPQ;
        ins.size = 2;
        ins.opcode = opcode;
        ins.ra = memory[cpu->pc + 1] >> 4;
        ins.rb = memory[cpu->pc + 1] & 0x0F;
        if (ins.rb != 0xf)
        {
            cpu->stat = INS;

            ins.type = INVALID;
            return ins;
        }
        break;
    default:
        ins.type = INVALID;
        ins.size = 0;
        ins.opcode = opcode;
        cpu->stat = INS;
        break;
    }

    // Finally, return the fetched instruction.
    return ins;
}

//============================================================================
/*
 * disassemble - disassemble a single instruction for the y86 architecture
 */
void disassemble(y86_inst_t inst)
{
    const char *reg_names[] = {
        "%rax", "%rcx", "%rdx", "%rbx", "%rsp", "%rbp", "%rsi",
        "%rdi", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13",
        "%r14", "%r15"};
    switch (inst.type)
    {
    case HALT:
        printf("halt\n");
        break;
    case NOP:
        printf("nop\n");
        break;
    case CMOV:
        printf("%s %s, %s\n", (inst.cmov == RRMOVQ) ? "rrmovq" : (inst.cmov == CMOVLE) ? "cmovle"
                                                             : (inst.cmov == CMOVL)    ? "cmovl"
                                                             : (inst.cmov == CMOVE)    ? "cmove"
                                                             : (inst.cmov == CMOVNE)   ? "cmovne"
                                                             : (inst.cmov == CMOVGE)   ? "cmovge"
                                                                                       : "cmovg",
               reg_names[inst.ra], reg_names[inst.rb]);
        break;
    case IRMOVQ:
        printf("irmovq 0x%lx, %s\n", inst.value, reg_names[inst.rb]);
        break;
    case RMMOVQ:
        if (inst.rb != 0xf)
        {
            printf("rmmovq %s, 0x%lx(%s)\n", reg_names[inst.ra], inst.d, reg_names[inst.rb]);
        }
        else
        {
            printf("rmmovq %s, 0x%lx\n", reg_names[inst.ra], inst.d);
        }
        break;
    case MRMOVQ:
        if (inst.rb != BADREG)
        {
            printf("mrmovq 0x%lx(%s), %s\n", inst.d, reg_names[inst.rb], reg_names[inst.ra]);
        }
        else
        {
            printf("mrmovq 0x%lx, %s\n", inst.d, reg_names[inst.ra]);
        }
        break;
    case OPQ:
        printf("%s %s, %s\n", (inst.op == ADD) ? "addq" : (inst.op == SUB) ? "subq"
                                                      : (inst.op == AND)   ? "andq"
                                                                           : "xorq",
               reg_names[inst.ra], reg_names[inst.rb]);
        break;
    case JUMP:
        printf("%s 0x%lx\n", (inst.jump == JMP) ? "jmp" : (inst.jump == JLE) ? "jle"
                                                      : (inst.jump == JL)    ? "jl"
                                                      : (inst.jump == JE)    ? "je"
                                                      : (inst.jump == JNE)   ? "jne"
                                                      : (inst.jump == JGE)   ? "jge"
                                                                             : "jg",
               inst.dest);
        break;
    case CALL:
        printf("call 0x%lx\n", inst.dest);
        break;
    case RET:
        printf("ret\n");
        break;
    case PUSHQ:
        printf("pushq %s\n", reg_names[inst.ra]);
        break;
    case POPQ:
        printf("popq %s\n", reg_names[inst.ra]);
        break;
    default:
        printf("invalid\n");
    }
}

//============================================================================
/*
 * disassemble_code - disassemble a segment of code in memory and print it
 */
void disassemble_code(memory_t memory, elf_phdr_t *phdr, elf_hdr_t *hdr)
{
    if (!memory || !phdr || !hdr)
    {
        fprintf(stderr, "Error: Invalid arguments to disassemble_code\n");
        return;
    }
    y86_t cpu;      // CPU struct to store "fake" PC
    y86_inst_t ins; // struct to hold fetched instruction
    cpu.pc = phdr->p_vaddr;
    bool started = false;
    // start at beginning of the segment
    if (phdr->p_type == CODE)
    {
        printf("  0x%03x:                      | .pos 0x%03x code\n", phdr->p_vaddr, phdr->p_vaddr);
        // iterate through the segment one instruction at a time
        while (cpu.pc < (phdr->p_vaddr + phdr->p_filesz))
        {
            if (!started)
            {
                if (cpu.pc == hdr->e_entry)
                {
                    printf("  0x%03lx:                      | _start:\n", cpu.pc);
                    started = true;
                }
            }
            ins = fetch(&cpu, memory); // stage 1: fetch instruction
            if (ins.type == INVALID)
            {
                printf("Invalid opcode: 0x%02x\n", ins.opcode);
                cpu.pc += 1;
                break;
            }

            // print the memory address
            printf("  0x%03x: ", (unsigned int)cpu.pc);

            // print the raw bytes of the instruction
            for (int i = 0; i < ins.size; i++)
            {
                printf("%02x", memory[cpu.pc + i]);
            }

            // print padding spaces for alignment
            for (int i = ins.size; i < 10; i++)
            {
                printf("  ");
            }

            printf(" |   ");
            disassemble(ins); // stage 2: print disassembly

            if (!started && cpu.pc == hdr->e_entry)
            {
                printf("  0x%lx:                      | _start:\n", cpu.pc);
                started = true;
            }
            cpu.pc += ins.size;
        }
    }
}

//============================================================================
/*
 * disassemble_data - disassemble a segment of data in memory and print it
 */
void disassemble_data(memory_t memory, elf_phdr_t *phdr)
{
    if (!memory || !phdr)
    {
        fprintf(stderr, "Error: Invalid arguments to disassemble_data\n");
        return;
    }

    if (phdr->p_type == DATA)
    {
        printf("  0x%3x:                      | .pos 0x%x data\n", phdr->p_vaddr, phdr->p_vaddr);

        bool all_zero = true;
        for (int j = 0; j < phdr->p_filesz; j++)
        {
            if (j % 8 == 0)
            {
                printf("  0x%01x: ", phdr->p_vaddr + j);
            }

            printf("%02x", memory[phdr->p_vaddr + j]);

            if (j % 8 == 7)
            {
                printf("     |   .quad 0x");
                for (int k = 0; k < 8; k++)
                {
                    all_zero = true;
                    if (memory[phdr->p_vaddr + j - k] != 0)
                    {
                        all_zero = false;
                        printf("%01x", memory[phdr->p_vaddr + j - k]);
                    }
                }
                if (all_zero)
                {
                    printf("0");
                }
                printf("\n");
            }
        }
    }
}

//============================================================================
/*
 * disassemble_rodata - disassemble a segment of rodata in memory and print it
 */
void disassemble_rodata(memory_t memory, elf_phdr_t *phdr)
{
    // checks for null paramters
    if (memory == NULL || phdr == NULL)
    {
        return;
    }
    y86_t cpu;
    uint32_t addr = phdr->p_vaddr;
    cpu.pc = addr;
    bool finished = false;
    int i;

    printf("  0x%03lx:  ", (uint64_t)cpu.pc);
    printf("%29s%03lx rodata\n", "| .pos 0x", (uint64_t)cpu.pc);

    while (cpu.pc < addr + phdr->p_filesz)
    {
        finished = false;
        printf("  0x%03lx: ", (uint64_t)cpu.pc);
        // executes until for first ten bytes or null is found
        for (i = cpu.pc; i < cpu.pc + 10; i++)
        {
            printf("%02x", memory[i]);
            if (memory[i] == 0x00)
            {
                finished = true;
                print_spacing(i - (cpu.pc - 1));
                break;
            }
        }
        // print hex in characters
        i = cpu.pc;
        printf(" |   .string \"");
        while (memory[i] != 0x00)
        {
            printf("%c", memory[i++]);
        }

        // get number of bytes contained in this data
        int bytes = (i - cpu.pc) + 1;
        printf("\"");

        // if more hex needs to be printed for this string
        if (!finished)
        {
            printf("\n");

            // offset by ten for the first ten bytes printed
            i = cpu.pc + 10;
            int j = 0;
            printf("  0x%03lx: ", (uint64_t)i);

            // loop until null character is found
            while (memory[i] != 0x00)
            {
                printf("%02x", memory[i++]);
                // if j is multiple of ten print | and newline
                if (++j % 10 == 0)
                {
                    printf(" |\n  0x%03lx: ", (uint64_t)i);
                }
            }
            printf("%02x", memory[i++]);
            print_spacing((j % 10) + 1);
            printf(" |");
        }
        cpu.pc += bytes;
        printf("\n");
    }
}
//============================================================================
/*
 * This function checks if the registers are valid
 */
bool is_valid_reg(y86_t *cpu, memory_t memory, y86_inst_t *ins)
{
    if (cpu == NULL || memory == NULL || ins == NULL)
    {
        return false;
    }

    ins->ra = memory[cpu->pc + 1] >> 4;
    ins->rb = memory[cpu->pc + 1] & 0x0F;

    if (ins->type != RMMOVQ && ins->type != MRMOVQ)
    {
        if (ins->ra < 0 || ins->ra > 15 || ins->rb < 0 || ins->rb > 15)
        {
            return false;
        }
    }

    return true;
}
//============================================================================
/*
 * This function checks that the program counter does not exceed the memory bounds.
 */
bool validate_pc(y86_t *cpu, int offset)
{
    return (cpu->pc + offset) < MEMSIZE;
}
//============================================================================
/*
 * This helper function sets the instruction to invalid and sets the status to ADR.
 */
y86_inst_t set_invalid_ins(y86_t *cpu, y86_inst_t *ins)
{
    cpu->stat = ADR;
    ins->type = INVALID;
    return *ins;
}
//============================================================================
/*
 * This function handles spaces for formatting pruposes.
 */
void print_spacing(int printed)
{
    // prints needed spaces for formatting
    int spaces = 10 - printed;
    for (int i = 0; i < spaces; i++)
    {
        printf("  ");
    }
}