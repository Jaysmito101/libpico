#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PICO_ELF_LOG(...) printf(__VA_ARGS__)

#define PICO_IMPLEMENTATION
#include "pico/picoElf.h"

#define MAX_SECTION_HEADERS 1024
#define MAX_PROGRAM_HEADERS 1024

const char *readFile(const char *path, size_t *outSize)
{
    FILE *file = fopen(path, "rb");
    if (!file) {
        printf("Error: Could not open file: %s\n", path);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = (char *)malloc(fileSize + 1);
    if (fread(buffer, 1, fileSize, file) != (size_t)fileSize) {
        printf("Error: Could not read file: %s\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }
    buffer[fileSize] = '\0';
    fclose(file);
    if (outSize) {
        *outSize = (size_t)fileSize;
    }
    return buffer;
}

typedef struct {
    const char *buffer;
    size_t size;
    picoElfHeader header;
    size_t programHeadersCount;
    picoElfProgramHeader programHeaders[MAX_PROGRAM_HEADERS];
    size_t sectionHeadersCount;
    picoElfSectionHeader sectionHeaders[MAX_SECTION_HEADERS];
} ElfContext;

bool loadElfFile(const char *path, ElfContext *ctx)
{
    ctx->buffer = readFile(path, &ctx->size);
    if (!ctx->buffer) {
        printf("Error: Could not read ELF file: %s\n", path);
        return false;
    }

    picoElfResult result = picoElfParseHeader((const picoElfUchar *)ctx->buffer, ctx->size, &ctx->header);
    if (result != PICO_ELF_RESULT_SUCCESS) {
        printf("Error: Could not parse ELF header: %s\n", picoElfResultToString(result));
        return false;
    }

    result = picoElfParseProgramHeaderTable(
        (const picoElfUchar *)ctx->buffer,
        ctx->size,
        &ctx->header,
        ctx->programHeaders,
        MAX_PROGRAM_HEADERS,
        &ctx->programHeadersCount);
    if (result != PICO_ELF_RESULT_SUCCESS) {
        printf("Error: Could not parse ELF program headers: %s\n", picoElfResultToString(result));
    }

    result = picoElfParseSectionHeaderTable(
        (const picoElfUchar *)ctx->buffer,
        ctx->size,
        &ctx->header,
        ctx->sectionHeaders,
        MAX_SECTION_HEADERS,
        &ctx->sectionHeadersCount);
    if (result != PICO_ELF_RESULT_SUCCESS) {
        printf("Error: Could not parse ELF section headers: %s\n", picoElfResultToString(result));
        return false;
    }

    return true;
}

void printElfInfo(ElfContext *ctx)
{
    printf("ELF header:\n");
    picoElfHeaderDebugPrint(2, &ctx->header);

    if (ctx->programHeadersCount > 0) {
        printf("ELF program headers (%zu):\n", ctx->programHeadersCount);
        for (size_t i = 0; i < ctx->programHeadersCount; ++i) {
            printf("Program Header %zu:\n", i);
            picoElfProgramHeaderDebugPrint(2, &ctx->programHeaders[i]);
            printf("\n");
        }
    }

    printf("ELF section headers (%zu):\n", ctx->sectionHeadersCount);
    const picoElfSectionHeader *sectionHeaderStringTable = ctx->header.sectionHeaderStringTableIndex < ctx->sectionHeadersCount && ctx->header.sectionHeaderStringTableIndex > 0
                                                               ? &ctx->sectionHeaders[ctx->header.sectionHeaderStringTableIndex]
                                                               : NULL;
    for (size_t i = 0; i < ctx->sectionHeadersCount; ++i) {
        printf(
            "Section Header %zu [%s]:\n",
            i,
            sectionHeaderStringTable
                ? picoElfStringTableEntry(
                      (const picoElfUchar *)ctx->buffer,
                      ctx->size,
                      sectionHeaderStringTable,
                      ctx->sectionHeaders[i].nameOffset)
                : "Unknown");
        picoElfSectionHeaderDebugPrint(2, &ctx->sectionHeaders[i]);

        if (ctx->sectionHeaders[i].type == PICO_ELF_SHT_SYMTAB || ctx->sectionHeaders[i].type == PICO_ELF_SHT_DYNSYM) {
            size_t symbolTableEntryCount                     = 0;
            picoElfSymbolTableEntry symbolTableEntries[1024] = {0};
            picoElfResult result                             = picoElfParseSymbolTable(
                (const picoElfUchar *)ctx->buffer,
                ctx->size,
                &ctx->header,
                &ctx->sectionHeaders[i],
                symbolTableEntries,
                1024,
                &symbolTableEntryCount);
            if (result != PICO_ELF_RESULT_SUCCESS) {
                printf("Error: Could not parse ELF symbol table: %s\n", picoElfResultToString(result));
                continue;
            }

            printf("%*sSymbol Table Entries (%zu):\n", 2, "", symbolTableEntryCount);
            for (size_t j = 0; j < symbolTableEntryCount; ++j) {
                printf(
                    "%*sSymbol Table Entry %zu [%s]:\n",
                    4, "", j, j != 0 ? picoElfStringTableEntry((const picoElfUchar *)ctx->buffer, ctx->size, &ctx->sectionHeaders[ctx->sectionHeaders[i].link], symbolTableEntries[j].nameOffset) : "Not Applicable");
                picoElfSymbolTableEntryDebugPrint(6, &symbolTableEntries[j]);
            }
        } else if (ctx->sectionHeaders[i].type == PICO_ELF_SHT_REL || ctx->sectionHeaders[i].type == PICO_ELF_SHT_RELA) {
            size_t relocationEntryCount                    = 0;
            picoElfRelocationEntry relocationEntries[4096] = {0};
            picoElfResult result                           = picoElfParseRelocationTable(
                (const picoElfUchar *)ctx->buffer,
                ctx->size,
                &ctx->header,
                &ctx->sectionHeaders[i],
                relocationEntries,
                4096,
                &relocationEntryCount);
            if (result != PICO_ELF_RESULT_SUCCESS) {
                printf("Error: Could not parse ELF relocation table: %s\n", picoElfResultToString(result));
                continue;
            }

            printf("%*sRelocation Table Entries (%zu):\n", 2, "", relocationEntryCount);
            for (size_t j = 0; j < relocationEntryCount; ++j) {
                printf("%*sRelocation Entry %zu:\n", 4, "", j);
                picoElfRelocationEntryDebugPrint(6, &relocationEntries[j]);
            }
        }

        printf("\n");
    }
}

void copyMinimalElf(ElfContext *ctx, const char *outPath)
{
    char *newBuffer = (char *)malloc(ctx->size);
    if (!newBuffer)
        return;
    memcpy(newBuffer, ctx->buffer, ctx->size);

    picoElfHeader newHeader                 = ctx->header;
    newHeader.sectionHeaderCount            = 0;
    newHeader.sectionHeaderOffset           = 0;
    newHeader.sectionHeaderStringTableIndex = 0;

    picoElfWriteHeader((picoElfUchar *)newBuffer, ctx->size, &newHeader);
    memset(newBuffer + ctx->header.sectionHeaderOffset, 0, ctx->header.sectionHeaderCount * ctx->header.sectionHeaderEntrySize);

    for (size_t i = 0; i < ctx->programHeadersCount; ++i) {
        picoElfWriteProgramHeader((picoElfUchar *)newBuffer, ctx->size, &newHeader, i, &ctx->programHeaders[i]);
    }

    FILE *out = fopen(outPath, "wb");
    if (out) {
        fwrite(newBuffer, 1, ctx->size, out);
        fclose(out);
    } else {
        printf("Error: Could not open output file %s\n", outPath);
    }
    free(newBuffer);
}

int main(int argc, char *argv[])
{
    printf("Hello, Pico!\n");

    if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s <input.elf> [output.elf]\n", argv[0]);
        printf("Parses an ELF file and prints its information. If an output file is provided, a minimal copy is written.\n");
        return 0;
    }

    ElfContext ctx = {0};
    if (!loadElfFile(argv[1], &ctx)) {
        return 1;
    }

    printElfInfo(&ctx);

    if (argc >= 3) {
        copyMinimalElf(&ctx, argv[2]);
    }

    if (ctx.buffer) {
        free((void *)ctx.buffer);
    }

    printf("Goodbye, Pico!\n");
    return 0;
}
