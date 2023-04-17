#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "p2-load.h"

/*
 * Print the usage message for this program.
 */
void usage_p2()
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
    printf("Options must not be repeated neither explicitly nor implicitly.\n");
}
//=======================================================================
/*
 * Read the header from the file and store it in the struct.
 * Return true if the header is valid, false otherwise.
 */
bool read_phdr(FILE *file, uint16_t offset, elf_phdr_t *phdr)
{
    if (phdr == NULL)
    {
        return false;
    }
    if (file == NULL)
    {
        return false;
    }
    fseek(file, offset, SEEK_SET);

    phdr->magic = (long)NULL;

    // read in the segment offset
    fread(&phdr->p_offset, sizeof(uint32_t), 1, file);
    // read in the segment file size
    fread(&phdr->p_filesz, sizeof(uint32_t), 1, file);
    // read in the segment virtual address
    fread(&phdr->p_vaddr, sizeof(uint32_t), 1, file);
    // read in the segment type
    fread(&phdr->p_type, sizeof(uint16_t), 1, file);
    // read in the segment flags
    fread(&phdr->p_flag, sizeof(uint16_t), 1, file);
    // read in the segment magic number
    fread(&phdr->magic, sizeof(uint32_t), 1, file);

    // check if the magic number is correct
    if (&phdr->magic == NULL || phdr->magic != 0XDEADBEEF)
    {
        return false;
    }
    return true;
}
//=======================================================================
/*
 * Read the program headers from the file and store them in the array.
 * Return true if the headers are valid, false otherwise.
 */
* /
    void dump_phdrs(uint16_t numphdrs, elf_phdr_t phdr[])
{
    printf("Segment   Offset    VirtAddr  FileSize  Type      Flag\n");
    // checks for data types and and permissions, prints appropriate string.
    char *data_type = "Data";
    char *permissions = "";
    bool stack = false;
    for (int i = 0; i < numphdrs; i++)
    {
        // CHECK DATA TYPE:

        // check for DATA DATA TYPE
        if (phdr[i].p_type == 0)
        {
            data_type = "DATA";
        }
        // Check for CODE DATA TYPE
        if (phdr[i].p_type == 1)
        {
            data_type = "CODE";
        }
        // Check for STACK DATA TYPE
        if (phdr[i].p_type == 2)
        {
            data_type = "STACK";
            stack = true;
        }

        // CHECK FOR PERMISSION TYPES:

        // READ
        if (phdr[i].p_flag - 4 == 0)
        {
            permissions = "R  ";
        }
        // WRITE
        if (phdr[i].p_flag - 2 == 0)
        {
            permissions = " W ";
        }
        // WRITE + EXECUTE
        if (phdr[i].p_flag - 3 == 0)
        {
            permissions = " WX";
        }
        // EXECUTE
        if (phdr[i].p_flag - 1 == 0)
        {
            permissions = "  X";
        }
        // READ + WRITE + EXECUTE
        if (phdr[i].p_flag - 4 == 3)
        {
            permissions = "RWX";
        }
        // READ + WRITE
        if (phdr[i].p_flag - 4 == 2)
        {
            permissions = "RW ";
        }
        // READ + EXECUTE
        if (phdr[i].p_flag - 4 == 1)
        {
            permissions = "R X";
        }
        if (!stack)
        {
            printf("  %02d      0x%04x    0x%04x    0x%04x    %s      %s\n", i, phdr[i].p_offset, phdr[i].p_vaddr, phdr[i].p_filesz, data_type, permissions);
        }
        else
        {
            printf("  %02d      0x%04x    0x%04x    0x%04x    %s     %s\n", i, phdr[i].p_offset, phdr[i].p_vaddr, phdr[i].p_filesz, data_type, permissions);
        }
    }
    printf("\n");
}
//=======================================================================
/*
 * Load the segments from the file into memory.
 * Return true if successful, false otherwise.
 */
bool load_segment(FILE *file, memory_t memory, elf_phdr_t phdr)
{
    if (file == NULL)
    {
        return false;
    }
    if (memory == NULL)
    {
        return false;
    }
    if (phdr.p_vaddr > MEMSIZE || phdr.p_vaddr < 0)
    {
        return false;
    }
    if (phdr.p_vaddr + phdr.p_filesz > MEMSIZE)
    {
        return false;
    }

    if (fseek(file, phdr.p_offset, SEEK_SET) == 0)
    {
        fread(memory + phdr.p_vaddr, phdr.p_filesz, 1, file);
        return true;
    }
    else
    {
        return false;
    }
}
//=======================================================================
/*
 * Dump the contents of memory from start to end.
 * If start is 0x0, then the first line should be "Contents of memory from 0000 to end:"
 * If end is MEMSIZE, then the last line should be "Contents of memory from start to ffff:"
 * If both start and end are 0x0, then the first line should be "Contents of memory:"
 */
void dump_memory(memory_t memory, uint16_t start, uint16_t end)
{
    printf("Contents of memory from %04x to %04x:\n", start, end);
    for (int i = start; i < end; i++)
    {
        if ((i - start) % 16 == 0)
        {
            printf("  %04x ", i);
        }
        if ((i - start) % 8 == 0)
        {
            printf(" ");
        }
        printf("%02x", memory[i]);
        if ((i - start) % 16 != 15)
        {
            printf(" ");
        }
        if ((i - start) % 16 == 15)
        {
            printf("\n");
        }
    }
    if (end != MEMSIZE || start == 0x0)
    {
        printf("\n");
    }
}
