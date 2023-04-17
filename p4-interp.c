#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4-interp.h"

y86_register_t opHandler(y86_register_t valB, y86_register_t *valA, y86_inst_t inst, y86_t *cpu);
y86_register_t checkForRegister(y86_rnum_t reg, y86_t *cpu);
bool jumpChecker(y86_jump_t jmp, y86_t *cpu);
bool checkCondition(y86_cmov_t mov, y86_t *cpu);
void writeBack(y86_rnum_t reg, y86_t *cpu, y86_register_t valE);
//=======================================================================
/*
 * usage_p4() - print usage information for this program
 */
void usage_p4()
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
    printf("  -e      Execute program\n");
    printf("  -E      Execute program (debug trace mode)\n");
    printf("Options must not be repeated neither explicitly nor implicitly.\n");
}

//=======================================================================
/*
 * parse_command_line_p4() - parse command line arguments
 *
 * This function parses the command line arguments and sets the
 * appropriate flags in the arguments.  It returns true if the
 * arguments are valid, false otherwise.
 *
 * argc: number of command line arguments
 * argv: array of command line arguments
 * header: pointer to header flag
 * segments: pointer to segments flag
 * membrief: pointer to membrief flag
 * memfull: pointer to memfull flag
 * disas_code: pointer to disas_code flag
 * disas_data: pointer to disas_data flag
 * exec_normal: pointer to exec_normal flag
 * exec_debug: pointer to exec_debug flag
 * file: pointer to file name
 *
 * Returns true if the arguments are valid, false otherwise.
 */
bool parse_command_line_p4(int argc, char **argv,
                           bool *header, bool *segments, bool *membrief,
                           bool *memfull, bool *disas_code, bool *disas_data,
                           bool *exec_normal, bool *exec_debug, char **file)
{
    // check for passed in null values
    if (argc <= 1 || argv == NULL || header == NULL ||
        membrief == NULL || segments == NULL || memfull == NULL || disas_code == NULL || disas_data == NULL || exec_debug == NULL || exec_normal == NULL)
    {
        usage_p4();
        return false;
    }

    *header = false;
    *segments = false;
    *membrief = false;
    *memfull = false;
    *disas_code = false;
    *disas_data = false;
    *exec_normal = false;
    *exec_debug = false;
    char *optionStr = "+hHafsmMDdeE";
    int opt;
    opterr = 0;
    bool printHelp = false;

    // loop through commmand line arguements
    while ((opt = getopt(argc, argv, optionStr)) != -1)
    {
        switch (opt)
        {
        case 'h':
            printHelp = true;
            break;
        case 'H':
            *header = true;
            break;
        case 'm':
            *membrief = true;
            break;
        case 'M':
            *memfull = true;
            break;
        case 's':
            *segments = true;
            break;
        case 'a':
            *header = true;
            *membrief = true;
            *segments = true;
            break;
        case 'f':
            *header = true;
            *memfull = true;
            *segments = true;
            break;
        case 'd':
            *disas_code = true;
            break;
        case 'D':
            *disas_data = true;
            break;
        case 'E':
            *exec_debug = true;
            break;
        case 'e':
            *exec_normal = true;
            break;
        default:
            usage_p4();
            return false;
        }
    }
    if (*exec_normal && *exec_debug)
    {
        *exec_normal = false;
    }
    // checks if the help command was found and returns true
    if (printHelp)
    {
        *header = false;
        usage_p4();
        return true;
    }
    else if (*membrief && *memfull)
    {
        // checks if both the memory full and memory brief
        // commands were found and returns false as this is invalid
        usage_p4();
        return false;
    }
    else
    {
        // sets the last command arg to the file char array
        *file = argv[optind];
        if (*file == NULL || argc > optind + 1)
        {
            // prints usage and returns false if no file name was given
            usage_p4();
            return false;
        }

        return true;
    }
}

//=======================================================================
/*
 * This function dumps the registers of the Y86 CPU.
 */
void dump_cpu(const y86_t *cpu)
{
    // print the cpu specifications
    printf("dump of Y86 CPU:\n");
    printf("  %%rip: %016lx   flags: SF%d ZF%d OF%d  ", cpu->pc, cpu->sf, cpu->zf, cpu->of);

    // switch for cpu status
    switch (cpu->stat)
    {
    case (1):
        printf("AOK\n");
        break;
    case (2):
        printf("HLT\n");
        break;
    case (3):
        printf("ADR\n");
        break;
    case (4):
        printf("INS\n");
        break;
    }
    printf("  %%rax: %016lx    %%rcx: %016lx\n", cpu->rax, cpu->rcx);
    printf("  %%rdx: %016lx    %%rbx: %016lx\n", cpu->rdx, cpu->rbx);
    printf("  %%rsp: %016lx    %%rbp: %016lx\n", cpu->rsp, cpu->rbp);
    printf("  %%rsi: %016lx    %%rdi: %016lx\n", cpu->rsi, cpu->rdi);
    printf("   %%r8: %016lx     %%r9: %016lx\n", cpu->r8, cpu->r9);
    printf("  %%r10: %016lx    %%r11: %016lx\n", cpu->r10, cpu->r11);
    printf("  %%r12: %016lx    %%r13: %016lx\n", cpu->r12, cpu->r13);
    printf("  %%r14: %016lx\n\n", cpu->r14);
}

//=======================================================================
/*
 * This function decodes and executes the instruction at the current PC.
 */
y86_register_t decode_execute(y86_t *cpu, bool *cond, const y86_inst_t *inst,
                              y86_register_t *valA)
{
    // check for null values
    if (cond == NULL || valA == NULL)
    {
        cpu->stat = INS;
        return 0;
    }

    // check for pc exceeding memsize
    if (cpu->pc > MEMSIZE || cpu->pc < 0)
    {

        cpu->stat = ADR;
        cpu->pc = 0xffffffffffffffff;
        return 0;
    }

    y86_register_t valE = 0;
    y86_register_t valB;

    // switch on instructon type
    switch (inst->type)
    {
    case (HALT):
        cpu->stat = HLT;
        break;

    case (NOP):
        break;

    case (CMOV):
        *valA = checkForRegister(inst->ra, cpu);
        valE = *valA;
        *cond = checkCondition(inst->cmov, cpu);
        break;

    case (IRMOVQ):
        valE = (uint64_t)inst->value;
        break;

    case (RMMOVQ):
        *valA = checkForRegister(inst->ra, cpu);
        valB = checkForRegister(inst->rb, cpu);
        valE = (uint64_t)inst->d + valB;
        break;

    case (MRMOVQ):
        valB = checkForRegister(inst->rb, cpu);
        valE = (uint64_t)inst->d + valB;
        break;

    case (OPQ):
        *valA = checkForRegister(inst->ra, cpu);
        valB = checkForRegister(inst->rb, cpu);
        valE = opHandler(valB, valA, *inst, cpu);
        break;

    case (JUMP):
        *cond = jumpChecker(inst->jump, cpu);
        break;

    case (CALL):
        valB = cpu->rsp;
        valE = valB - 8;
        break;

    case (RET):
        *valA = cpu->rsp;
        valB = cpu->rsp;
        valE = valB + 8;
        break;

    case (PUSHQ):
        *valA = checkForRegister(inst->ra, cpu);
        valB = cpu->rsp;
        valE = valB - 8;
        break;

    case (POPQ):
        *valA = cpu->rsp;
        valB = cpu->rsp;
        valE = valB + 8;
        break;

    case (INVALID):
        cpu->stat = INS;
        break;
    default:
        cpu->stat = INS;
        break;
    }

    return valE;
}

//=======================================================================
/*
 * This function handles the memory and writeback stages of the CPU.
 */
* /
    void memory_wb_pc(y86_t *cpu, memory_t memory, bool cond,
                      const y86_inst_t *inst, y86_register_t valE,
                      y86_register_t valA)
{
    y86_register_t valM;
    uint64_t *p;

    // check for pc exceeding memsize
    if (cpu->pc > MEMSIZE || memory == NULL)
    {
        cpu->stat = ADR;
        cpu->pc = 0xffffffffffffffff;
        return;
    }

    // switch on instruction type
    switch (inst->type)
    {
    case (HALT):
        cpu->pc += inst->size;
        cpu->zf = false;
        cpu->sf = false;
        cpu->of = false;
        break;

    case (NOP):
        cpu->pc += inst->size;
        break;

    case (CMOV):
        if (cond)
        {
            writeBack(inst->rb, cpu, valE);
        }
        cpu->pc += inst->size;
        break;

    case (IRMOVQ):
        writeBack(inst->rb, cpu, valE);
        cpu->pc += inst->size;
        break;

    case (RMMOVQ):
        if (valE > MEMSIZE)
        {
            cpu->stat = ADR;
            cpu->pc = 0xffffffffffffffff;
            break;
        }
        p = (uint64_t *)&memory[valE];
        *p = valA;
        cpu->pc += inst->size;
        printf("Memory write to 0x%04lx: 0x%lx\n", valE, valA);
        break;

    case (MRMOVQ):
        // Check if starting and ending addresses are within the valid range
        if (valE < 0 || valE >= MEMSIZE)
        {
            cpu->stat = ADR;
            cpu->pc += inst->size;
            break;
        }
        p = (uint64_t *)&memory[valE];
        valM = *p;
        writeBack(inst->ra, cpu, valM);
        cpu->pc += inst->size;
        break;

    case (OPQ):
        writeBack(inst->rb, cpu, valE);
        cpu->pc += inst->size;
        break;

    case (JUMP):
        if (cond)
        {
            cpu->pc = inst->dest;
            break;
        }
        cpu->pc += inst->size;
        break;

    case (CALL):
        if (valE >= MEMSIZE)
        {
            cpu->stat = ADR;
            cpu->pc = inst->dest;
            break;
        }
        p = (uint64_t *)&memory[valE];
        *p = cpu->pc += inst->size;
        cpu->rsp = valE;
        cpu->pc = inst->dest;
        printf("Memory write to 0x%04lx: 0x%lx\n", valE, *p);
        break;

    case (RET):
        if (valA >= MEMSIZE)
        {
            cpu->stat = ADR;
            cpu->pc = 0xffffffffffffffff;
            break;
        }
        p = (uint64_t *)&memory[valA];
        valM = *p;
        cpu->rsp = valE;
        cpu->pc = valM;
        break;

    case (PUSHQ):
        if (valE >= MEMSIZE)
        {
            cpu->stat = ADR;
            cpu->pc = 0xffffffffffffffff;
            break;
        }
        p = (uint64_t *)&memory[valE];
        *p = valA;
        cpu->rsp = valE;
        cpu->pc += inst->size;
        printf("Memory write to 0x%04lx: 0x%lx\n", valE, valA);
        break;

    case (POPQ):
        if (valA >= MEMSIZE)
        {
            cpu->stat = ADR;
            cpu->pc = 0xffffffffffffffff;
            break;
        }
        p = (uint64_t *)&memory[valA];
        valM = *p;
        cpu->rsp = valE;
        writeBack(inst->ra, cpu, valM);
        cpu->pc += inst->size;
        break;

    case (INVALID):
        cpu->stat = INS;
        break;
    }
}
//=======================================================================
// This function should compute the XOR value of two boolean flags in the y-86 isa and return the proper boolean. (XOR does not behave properly with XOR)
bool booleanXOR(bool first, bool second)
{
    return (first && !second) || (!first && second);
}

//=======================================================================
/*
 * This function handles the ALU operations for the CPU.
 */
y86_register_t opHandler(y86_register_t valB, y86_register_t *valA, y86_inst_t inst, y86_t *cpu)
{
    int64_t signedValE = 0;
    y86_register_t valE = 0;
    int64_t signedValB = valB;
    int64_t signedValA = *valA;

    // Check flags for opq instruction
    switch (inst.op)
    {
    case ADD:
        signedValE = signedValB + signedValA;
        // Set overflow of addition
        cpu->of = ((signedValB < 0) && (signedValA < 0) && (signedValE > 0)) ||
                  ((signedValB > 0) && (signedValA > 0) && (signedValE < 0));
        valE = signedValE;
        break;

    case SUB:
        signedValE = signedValB - signedValA;
        // Set overflow of subtraction
        cpu->of = ((signedValB < 0) && (signedValA > 0) && (signedValE > 0)) ||
                  ((signedValB > 0) && (signedValA < 0) && (signedValE < 0));
        valE = signedValE;
        break;

    case AND:
        valE = valB & *valA;
        cpu->of = 0;
        break;

    case XOR:
        valE = *valA ^ valB;
        cpu->of = 0;
        break;

    case BADOP:
        cpu->stat = INS;
        return valE;

    default:
        cpu->stat = INS;
        return valE;
    }

    // Set sign or zero flag for all cases
    cpu->sf = (valE >> 63) == 1;
    cpu->zf = valE == 0;

    return valE;
}

//=======================================================================
/*
 * This function checks if the register is valid and returns the value of the register.
 */
y86_register_t checkForRegister(y86_rnum_t reg, y86_t *cpu)
{
    // helper mehtod switch statment to retrieve register
    switch (reg)
    {
    case (0):
        return cpu->rax;
    case (1):
        return cpu->rcx;
    case (2):
        return cpu->rdx;
    case (3):
        return cpu->rbx;
    case (4):
        return cpu->rsp;
    case (5):
        return cpu->rbp;
    case (6):
        return cpu->rsi;
    case (7):
        return cpu->rdi;
    case (8):
        return cpu->r8;
    case (9):
        return cpu->r9;
    case (10):
        return cpu->r10;
    case (11):
        return cpu->r11;
    case (12):
        return cpu->r12;
    case (13):
        return cpu->r13;
    case (14):
        return cpu->r14;
    case (BADREG):
        return 0;
    default:
        cpu->stat = INS;
        return 0;
    }
}

//=======================================================================
/*
 * This function checks if a condition is met for a cmov instruction.
 */
bool checkCondition(y86_cmov_t mov, y86_t *cpu)
{
    // switch statement helper method to check cmov condition
    bool cond = false;
    switch (mov)
    {
    case (RRMOVQ):
        cond = true;
        break;
    case (CMOVLE):
        if (cpu->zf || booleanXOR(cpu->sf, cpu->of))
        {
            cond = true;
        }
        break;
    case (CMOVL):
        if (booleanXOR(cpu->sf, cpu->of))
        {
            cond = true;
        }
        break;
    case (CMOVE):
        if (cpu->zf)
        {
            cond = true;
        }
        break;
    case (CMOVNE):
        if (!cpu->zf)
        {
            cond = true;
        }
        break;
    case (CMOVGE):
        if (cpu->sf == cpu->of)
        {
            cond = true;
        }
        break;
    case (CMOVG):
        if (!cpu->zf && cpu->sf == cpu->of)
        {
            cond = true;
        }
        break;
    case (BADCMOV):
        cpu->stat = INS;
    }

    return cond;
}

//=======================================================================
/*
 * This function checks if a condition is met for a jump instruction.
 */
bool jumpChecker(y86_jump_t jmp, y86_t *cpu)
{
    // switch statement helper method to check jump condition
    bool cond = false;
    switch (jmp)
    {
    case (JMP):
        cond = true;
        break;
    case (JLE):
        if (cpu->zf || booleanXOR(cpu->sf, cpu->of))
        {
            cond = true;
        }
        break;
    case (JL):
        if (booleanXOR(cpu->sf, cpu->of))
        {
            cond = true;
        }
        break;
    case (JE):
        if (cpu->zf)
        {
            cond = true;
        }
        break;
    case (JNE):
        if (!cpu->zf)
        {
            cond = true;
        }
        break;
    case (JGE):
        if (cpu->sf == cpu->of)
        {
            cond = true;
        }
        break;
    case (JG):
        if (!cpu->zf && cpu->sf == cpu->of)
        {
            cond = true;
        }
        break;
    case (BADJUMP):
        cpu->stat = INS;
    }

    return cond;
}

//=======================================================================
/*
 * This function writes back the value to the register.
 */
void writeBack(y86_rnum_t reg, y86_t *cpu, y86_register_t value)
{
    // helper method to write back value to register
    switch (reg)
    {
    case (0):
        cpu->rax = value;
        break;
    case (1):
        cpu->rcx = value;
        break;
    case (2):
        cpu->rdx = value;
        break;
    case (3):
        cpu->rbx = value;
        break;
    case (4):
        cpu->rsp = value;
        break;
    case (5):
        cpu->rbp = value;
        break;
    case (6):
        cpu->rsi = value;
        break;
    case (7):
        cpu->rdi = value;
        break;
    case (8):
        cpu->r8 = value;
        break;
    case (9):
        cpu->r9 = value;
        break;
    case (10):
        cpu->r10 = value;
        break;
    case (11):
        cpu->r11 = value;
        break;
    case (12):
        cpu->r12 = value;
        break;
    case (13):
        cpu->r13 = value;
        break;
    case (14):
        cpu->r14 = value;
        break;
    case (BADREG):
        cpu->stat = INS;
    default:
        break;
    }
}