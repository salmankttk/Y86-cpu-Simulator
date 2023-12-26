#ifndef __VALID__
#define __VALID__

#include <stdbool.h>

#include "elf.h"

void usage_val ();
bool parse_command_line_p1 ( int argc, char **argv, bool *header, char **file );

bool read_header ( FILE *file, elf_hdr_t *hdr );
void dump_header ( elf_hdr_t hdr );

#endif
