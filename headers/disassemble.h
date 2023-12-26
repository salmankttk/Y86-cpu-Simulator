#ifndef __DISS__
#define __DISS__
 
#include <stdbool.h>
#include <string.h>

#include "elf.h"
#include "y86.h"

void usage_dis ();
bool parse_command_line_p3 (int argc, char **argv,
        bool *header, bool *segments, bool *membrief, bool *memfull,
        bool *disas_code, bool *disas_data, char **file);

y86_inst_t fetch (y86_t *cpu, memory_t memory);

void disassemble (y86_inst_t inst);
void disassemble_code   (memory_t memory, elf_phdr_t *phdr, elf_hdr_t *hdr);
void disassemble_data   (memory_t memory, elf_phdr_t *phdr);
void disassemble_rodata (memory_t memory, elf_phdr_t *phdr);

#endif
