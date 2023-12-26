#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "./headers/validate-header.h"
// Needed to check validity of magic number across endian storage methods.
#include <netinet/in.h>
/*
 * Print the usage message for this program.
 */
void usage_val()
{
    printf("Usage: y86 <option(s)> mini-elf-file\n");
    printf(" Options are:\n");
    printf("  -h      Display usage\n");
    printf("  -H      Show the Mini-ELF header\n");
    printf("Options must not be repeated neither explicitly nor implicitly.\n");
}
//=======================================================================
/*
 * Read the header from the file and store it in the struct.
 * Return true if the header is valid, false otherwise.
 */
bool read_header(FILE *file, elf_hdr_t *hdr)
{
    // null checks
    if (file == NULL)
    {
        return false;
    }
    if (hdr == NULL)
    {
        return false;
    }
    fseek(file, 0, SEEK_END);
    // check if the size of mini-ELF is correct.
    int size = ftell(file);
    if (size < 16)
    {
        return false;
    }
    // read in the struct
    fseek(file, 0, SEEK_SET);
    fread(&hdr->e_version, sizeof(hdr->e_version), 1, file);
    fread(&hdr->e_entry, sizeof(hdr->e_entry), 1, file);
    fread(&hdr->e_phdr_start, sizeof(hdr->e_phdr_start), 1, file);
    fread(&hdr->e_num_phdr, sizeof(hdr->e_num_phdr), 1, file);
    fread(&hdr->e_symtab, sizeof(hdr->e_symtab), 1, file);
    fread(&hdr->e_strtab, sizeof(hdr->e_strtab), 1, file);
    fread(&hdr->magic, sizeof(hdr->magic), 1, file);
    // check magic number.
    if (hdr->magic != ntohl(0x454c4600))
    {
        return false;
    }
    return true;
}
//=======================================================================
/*
 * Print the header in the format specified in the assignment.
 */
void dump_header(elf_hdr_t hdr)
{
    // print mamory location.
    printf("00000000  ");
    // formatted print for all hex values in header.
    for (int i = 0; i < sizeof(hdr); i++)
    {
        if (i == sizeof(hdr) - 1)
        {
            printf("%02x", ((unsigned char *)&hdr)[i]);
            break;
        }
        printf("%02x ", ((unsigned char *)&hdr)[i]);
        if (i == 7)
        {
            printf(" ");
        }
    }
    printf("\n");
}
