/*
 * A Y-86 simulator constructed in C-99
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "./headers/validate-header.h"
#include "./headers/mem-access.h"
#include "./headers/disassemble.h"
#include "./headers/interpret.h"

int main(int argc, char **argv)
{
    // boolean variables used to identify flags:
    bool header = false;
    bool segments = false;
    bool membrief = false;
    bool memfull = false;
    bool disas_code = false;
    bool disas_data = false;
    bool exec_normal = false;
    bool exec_debug = false;

    char *file = NULL;
    FILE *fileOpen;
    // header struct... free at end.
    elf_hdr_t *hdr = malloc(sizeof(elf_hdr_t));

    // command-line parser
    if (parse_command_line_p4(argc, argv, &header, &segments, &membrief, &memfull, &disas_code, &disas_data, &exec_normal, &exec_debug, &file) == false)
    {

        exit(EXIT_FAILURE);
    }

    // if header is false, exit the program.
    if (!header && file == NULL)
    {
        exit(EXIT_SUCCESS);
    }

    // open File... Close at end.
    fileOpen = fopen(file, "r");

    // Check that file is readable.
    if (fileOpen)
    {
    }
    else
    {
        printf("Failed to open File\n");
        exit(EXIT_FAILURE);
    }

    // Read in the header.
    if (read_header(fileOpen, hdr))
    {
        // if the header option is selected, print the mini_elf metadata.
        if (header)
        {
            dump_header(*hdr);
            printf("Mini-ELF version %x\n", hdr->e_version);
            printf("Entry point 0x%x\n", hdr->e_entry);
            printf("There are %x program headers, starting at offset %d (0x%x)\n", hdr->e_num_phdr, hdr->e_phdr_start, hdr->e_phdr_start);
            if (hdr->e_symtab != 0)
            {
                printf("There is a symbol table starting at offset %d (0x%x)\n", hdr->e_symtab, hdr->e_symtab);
            }
            else
            {
                printf("There is no symbol table present\n");
            }
            if (hdr->e_strtab != 0)
            {
                printf("There is a string table starting at offset %d (0x%x)\n", hdr->e_strtab, hdr->e_strtab);
            }
            else
            {
                printf("There is no string table present\n");
            }
            printf("\n");
        }
    }
    else
    {
        printf("Failed to Read ELF Header\n");
        exit(EXIT_FAILURE);
    }

    // create an array of program headers
    elf_phdr_t *phdr = malloc(sizeof(elf_phdr_t) * hdr->e_num_phdr);

    // create a virtual memory to allocate information to. The size is MEMSIZE.
    memory_t mem = calloc(MEMSIZE, sizeof(memory_t));

    // read in each program header, if an invalid header is encountered, exit the program with a status error.
    for (int i = 0; i < hdr->e_num_phdr; i++)
    {
        if (read_phdr(fileOpen, hdr->e_phdr_start + (i * sizeof(elf_phdr_t)), &phdr[i]))
        {
            if (load_segment(fileOpen, mem, phdr[i]))
            {
            }
            else
            {
                printf("Failed to Load Segment");
                fclose(fileOpen);
                free(hdr);
                free(mem);
                free(phdr);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            printf("Failed to Read Program Header\n");
            fclose(fileOpen);
            free(hdr);
            free(mem);
            free(phdr);
            exit(EXIT_FAILURE);
        }
    }

    // segments flag (-s) This flag dumps the program headers.
    if (segments)
    {
        dump_phdrs(hdr->e_num_phdr, phdr);
    }

    // membreif flag (-m) This flag dumps the virtual memory being used by the program.
    if (membrief)
    {
        int correction = 0;
        for (int i = 0; i < hdr->e_num_phdr; i++)
        {
            if (phdr[i].p_filesz != 0)
            {
                if (phdr[i].p_vaddr % 16 != 0)
                {
                    correction = phdr[i].p_vaddr - (phdr[i].p_vaddr % 16);
                }
                else
                {
                    correction = phdr[i].p_vaddr;
                }
                dump_memory(mem, correction, phdr[i].p_vaddr + phdr[i].p_filesz);
                printf("\n");
            }
        }
    }

    // memfull (-M) This flag dumps the full virtual memory.
    if (memfull)
    {
        dump_memory(mem, 0, MEMSIZE);
    }

    // disassemble code (-d) This flag will diassemble all the code sections stored in virutal memory.
    if (disas_code)
    {
        printf("Disassembly of executable contents:\n");
        for (int i = 0; i < hdr->e_num_phdr; i++)
        {
            if (phdr[i].p_type == CODE)
            {
                disassemble_code(mem, &phdr[i], hdr);
                printf("\n");
            }
        }
    }

    // disassemble data (-D) This flag will disassemble all the data sections stored in virtual memory.
    if (disas_data)
    {
        printf("Disassembly of data contents:\n");

        // Iterate through the program header array to find the data segment entry
        for (int i = 0; i < hdr->e_num_phdr; i++)
        {
            if (phdr[i].p_type == DATA) // Assuming that the correct value for the data segment is defined as DATA
            {
                // If the data is read only, send it to disassemble_rodata
                if (phdr[i].p_flag == 0x4)
                {
                    disassemble_rodata(mem, &phdr[i]);
                    printf("\n");
                }
                else
                {
                    disassemble_data(mem, &phdr[i]);
                    printf("\n");
                }
            }
        }
    }

    // Initialize a cpu, set its status to AOK, and assign it to the entry-point of the program.
    y86_t cpu;
    memset(&cpu, 0x00, sizeof(cpu));
    cpu.stat = AOK;
    int count = 0;
    y86_register_t valA = 0;
    y86_register_t valE = 0;
    bool cond = false;
    cpu.pc = hdr->e_entry;

    // normal Execution (-e) This flag will execute all instructions in "normal" mode.
    if (exec_normal)
    {
        printf("Entry execution point at 0x%04x\n", hdr->e_entry);
        printf("Initial ");
        dump_cpu(&cpu);

        // loop executes while cpu status is ok
        while (cpu.stat == AOK)
        {
            // fetch the instruction
            y86_inst_t ins = fetch(&cpu, mem);
            cond = false;
            // debug and execute the instruction
            valE = decode_execute(&cpu, &cond, &ins, &valA);

            // If invalid opcode, print the relevant message
            if (cpu.stat == INS)
            {
                printf("Corrupt Instruction (opcode 0x%02x) at address 0x%04lx\n", ins.opcode, cpu.pc);
            }

            // write value values to memory and regisiters
            // update program counter
            memory_wb_pc(&cpu, mem, cond, &ins, valE, valA);
            count++;

            // check to that pc didnt exceed memsize
            if (cpu.pc >= MEMSIZE)
            {
                cpu.stat = ADR;
                cpu.pc = 0xffffffffffffffff;
            }
        }

        if (cpu.stat == INS)
        {
            printf("Post-Fetch ");
        }
        else
        {
            printf("Post-Exec ");
        }
        dump_cpu(&cpu);

        // print cpu state
        printf("Total execution count: %d instructions\n\n", count);
    }

    // Debug execution (-E) This flag will execute all instructions in "debug" mode, it will additionally
    // print the entire memory contents at the end of execution.
    if (exec_debug)
    {
        printf("Entry execution point at 0x%04x\n", hdr->e_entry);

        for (int i = 0; i < hdr->e_num_phdr; i++)
        {
            if (i == 0)
            {
                printf("Initial ");
                dump_cpu(&cpu);
            }
            // execute while cpu status is ok
            while (cpu.stat == AOK)
            {
                // fetch intsruction
                y86_inst_t ins = fetch(&cpu, mem);

                // print instruction
                printf("Executing: ");
                disassemble(ins);

                // decode and execute
                valE = decode_execute(&cpu, &cond, &ins, &valA);

                // If invalid opcode, print the relevant message
                if (cpu.stat == INS)
                {
                    printf("Corrupt Instruction (opcode 0x%02x) at address 0x%04lx\n", ins.opcode, cpu.pc);
                }

                // write value to memory and registers
                // update program counter
                memory_wb_pc(&cpu, mem, cond, &ins, valE, valA);
                count++;

                if (cpu.stat == INS)
                {
                    printf("Post-Fetch ");
                }
                else
                {
                    printf("Post-Exec ");
                }
                dump_cpu(&cpu);
            }
        }
        // print cpu status
        printf("Total execution count: %d instructions\n\n", count);
        dump_memory(mem, 0, MEMSIZE);
    }

    // close and free memory.
    fclose(fileOpen);
    free(hdr);
    free(mem);
    free(phdr);

    return EXIT_SUCCESS;
}