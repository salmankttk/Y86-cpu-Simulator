
#ifndef __INTERP__
#define __INTERP__

#include <stdbool.h>
#include <string.h>

#include "elf.h"
#include "y86.h"

void usage_interp ();
bool parse_command_line_p4 (int argc, char **argv,
        bool *header, bool *segments, bool *membrief, bool *memfull,
        bool *disas_code, bool *disas_data,
        bool *exec_normal, bool *exec_debug, char **file );

void dump_cpu( const y86_t *cpu ) ;

y86_register_t decode_execute(  y86_t *cpu , bool *cond , const y86_inst_t *inst ,
                                y86_register_t *valA 
                             ) ;

void memory_wb_pc(  y86_t *cpu , memory_t memory , bool cond , 
                    const y86_inst_t *inst , y86_register_t  valE , 
                    y86_register_t  valA 
                 ) ;

#endif
