/*
MIT License

Copyright (c) 2026 Jaysmito Mukherjee (jaysmito101@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef PICO_ELF_H
#define PICO_ELF_H

#include <inttypes.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PICO_MALLOC
#define PICO_MALLOC(sz) malloc(sz)
#define PICO_FREE(ptr)  free(ptr)
#endif

#ifndef PICO_ELF_LOG
#define PICO_ELF_LOG(...) \
    do {                  \
        (void)0;          \
    } while (0);
#endif

#ifndef PICO_ASSERT
#include <assert.h>
#define PICO_ASSERT(expr) assert(expr)
#endif

#define PICO_ELF_EI_NIDENT 16

typedef uint32_t picoElf32Addr;
typedef uint16_t picoElf32Half;
typedef uint32_t picoElf32Off;
typedef int32_t picoElf32Sword;
typedef uint32_t picoElf32Word;
typedef uint8_t picoElfUchar;

typedef uint64_t picoElf64Addr;
typedef uint64_t picoElf64Off;
typedef uint16_t picoElf64Half;
typedef uint32_t picoElf64Word;
typedef int32_t picoElf64Sword;
typedef uint64_t picoElf64Xword;
typedef int64_t picoElf64Sxword;

typedef enum {
    PICO_ELF_RESULT_SUCCESS = 0,
    PICO_ELF_RESULT_INVALID_MAGIC,
    PICO_ELF_RESULT_UNSUPPORTED_CLASS,
    PICO_ELF_RESULT_UNSUPPORTED_DATA_ENCODING,
    PCIO_ELF_RESULT_COUNT
} picoElfResult;

typedef enum {
    PICO_ELF_CLASS_NONE = 0, // Invalid class
    PICO_ELF_CLASS_32   = 1, // 32-bit objects
    PICO_ELF_CLASS_64   = 2  // 64-bit objects
} picoElfClass;

typedef enum {
    PICO_ELF_DATA_NONE = 0,
    PICO_ELF_DATA_LSB  = 1,
    PICO_ELF_DATA_MSB  = 2
} picoElfDataEncoding;

typedef enum {
    PICO_ELF_EV_NONE    = 0, // Invalid version
    PICO_ELF_EV_CURRENT = 1  // Current version
} picoElfVersion;

typedef enum {
    PICO_ELF_OSABI_NONE       = 0,  // System V ABI
    PICO_ELF_OSABI_HPUX       = 1,  // HP-UX
    PICO_ELF_OSABI_NETBSD     = 2,  // NetBSD
    PICO_ELF_OSABI_LINUX      = 3,  // Linux
    PICO_ELF_OSABI_SOLARIS    = 6,  // Solaris
    PICO_ELF_OSABI_AIX        = 7,  // AIX
    PICO_ELF_OSABI_IRIX       = 8,  // IRIX
    PICO_ELF_OSABI_FREEBSD    = 9,  // FreeBSD
    PICO_ELF_OSABI_TRU64      = 10, // Compaq TRU64 UNIX
    PICO_ELF_OSABI_MODESTO    = 11, // Novell Modesto
    PICO_ELF_OSABI_OPENBSD    = 12, // OpenBSD
    PICO_ELF_OSABI_OPENVMS    = 13, // OpenVMS
    PICO_ELF_OSABI_NSK        = 14, // Hewlett-Packard Non-Stop Kernel
    PICO_ELF_OSABI_ARM_AEABI  = 64, // ARM EABI
    PICO_ELF_OSABI_ARM        = 97, // ARM
    PICO_ELF_OSABI_STANDALONE = 255 // Standalone application
} picoElfOSABI;

typedef struct {
    picoElfUchar magic[4]; // Always 0x7F 'E' 'L' 'F'
    picoElfClass elfClass;
    picoElfDataEncoding dataEncoding;
    picoElfVersion version;
    picoElfOSABI osABI;
    picoElfUchar abiVersion;
} picoElfIdentifier;

typedef enum {
    PICO_ELF_ET_NONE   = 0,      // No file type
    PICO_ELF_ET_REL    = 1,      // Relocatable file
    PICO_ELF_ET_EXEC   = 2,      // Executable file
    PICO_ELF_ET_DYN    = 3,      // Shared object file
    PICO_ELF_ET_CORE   = 4,      // Core file
    PICO_ELF_ET_LOPROC = 0xff00, // Processor-specificl
    PICO_ELF_ET_HIPROC = 0xffff  // Processor-specific
} picoElfType;

typedef enum {
    PICO_ELF_EM_NONE        = 0,   // No machine
    PICO_ELF_EM_M32         = 1,   // AT&T WE 32100
    PICO_ELF_EM_SPARC       = 2,   // SPARC
    PICO_ELF_EM_386         = 3,   // Intel 80386
    PICO_ELF_EM_68K         = 4,   // Motorola 68000
    PICO_ELF_EM_88K         = 5,   // Motorola 88000
    PICO_ELF_EM_860         = 7,   // Intel 80860
    PICO_ELF_EM_MIPS        = 8,   // MIPS I Architecture
    PICO_ELF_EM_S370        = 9,   // IBM System/370 Processor
    PICO_ELF_EM_MIPS_RS3_LE = 10,  // MIPS RS3000 Little-endian
    PICO_ELF_EM_PARISC      = 15,  // Hewlett-Packard PA-RISC
    PICO_ELF_EM_VPP500      = 17,  // Fujitsu VPP500
    PICO_ELF_EM_SPARC32PLUS = 18,  // Enhanced instruction set SPARC
    PICO_ELF_EM_960         = 19,  // Intel 80960
    PICO_ELF_EM_PPC         = 20,  // PowerPC
    PICO_ELF_EM_PPC64       = 21,  // 64-bit PowerPC
    PICO_ELF_EM_S390        = 22,  // IBM System/390 Processor
    PICO_ELF_EM_V800        = 36,  // NEC V800
    PICO_ELF_EM_FR20        = 37,  // Fujitsu FR20
    PICO_ELF_EM_RH32        = 38,  // TRW RH-32
    PICO_ELF_EM_RCE         = 39,  // Motorola RCE
    PICO_ELF_EM_ARM         = 40,  // Advanced RISC Machines ARM
    PICO_ELF_EM_ALPHA       = 41,  // Digital Alpha
    PICO_ELF_EM_SH          = 42,  // Hitachi SH
    PICO_ELF_EM_SPARCV9     = 43,  // SPARC Version 9
    PICO_ELF_EM_TRICORE     = 44,  // Siemens TriCore embedded processor
    PICO_ELF_EM_ARC         = 45,  // Argonaut RISC Core, Argonaut Technologies Inc.
    PICO_ELF_EM_H8_300      = 46,  // Hitachi H8/300
    PICO_ELF_EM_H8_300H     = 47,  // Hitachi H8/300H
    PICO_ELF_EM_H8S         = 48,  // Hitachi H8S
    PICO_ELF_EM_H8_500      = 49,  // Hitachi H8/500
    PICO_ELF_EM_IA_64       = 50,  // Intel IA-64 processor architecture
    PICO_ELF_EM_MIPS_X      = 51,  // Stanford MIPS-X
    PICO_ELF_EM_COLDFIRE    = 52,  // Motorola ColdFire
    PICO_ELF_EM_68HC12      = 53,  // Motorola M68HC12
    PICO_ELF_EM_MMA         = 54,  // Fujitsu MMA Multimedia Accelerator
    PICO_ELF_EM_PCP         = 55,  // Siemens PCP
    PICO_ELF_EM_NCPU        = 56,  // Sony nCPU embedded RISC processor
    PICO_ELF_EM_NDR1        = 57,  // Denso NDR1 microprocessor
    PICO_ELF_EM_STARCORE    = 58,  // Motorola Star*Core processor
    PICO_ELF_EM_ME16        = 59,  // Toyota ME16 processor
    PICO_ELF_EM_ST100       = 60,  // STMicroelectronics ST100 processor
    PICO_ELF_EM_TINYJ       = 61,  // Advanced Logic Corp. TinyJ embedded processor family
    PICO_ELF_EM_X86_64      = 62,  // AMD x86-64 architecture
    PICO_ELF_EM_PDSP        = 63,  // Sony DSP Processor
    PICO_ELF_EM_PDP10       = 64,  // Digital Equipment Corp. PDP-10
    PICO_ELF_EM_PDP11       = 65,  // Digital Equipment Corp. PDP-11
    PICO_ELF_EM_FX66        = 66,  // Siemens FX66 microcontroller
    PICO_ELF_EM_ST9PLUS     = 67,  // STMicroelectronics ST9+ 8/16 bit microcontroller
    PICO_ELF_EM_ST7         = 68,  // STMicroelectronics ST7 8-bit microcontroller
    PICO_ELF_EM_68HC16      = 69,  // Motorola MC68HC16 Microcontroller
    PICO_ELF_EM_68HC11      = 70,  // Motorola MC68HC11 Microcontroller
    PICO_ELF_EM_68HC08      = 71,  // Motorola MC68HC08 Microcontroller
    PICO_ELF_EM_68HC05      = 72,  // Motorola MC68HC05 Microcontroller
    PICO_ELF_EM_SVX         = 73,  // Silicon Graphics SVx
    PICO_ELF_EM_ST19        = 74,  // STMicroelectronics ST19 8-bit microcontroller
    PICO_ELF_EM_VAX         = 75,  // Digital VAX
    PICO_ELF_EM_CRIS        = 76,  // Axis Communications 32-bit embedded processor
    PICO_ELF_EM_JAVELIN     = 77,  // Infineon Technologies 32-bit embedded processor
    PICO_ELF_EM_FIREPATH    = 78,  // Element 14 64-bit DSP Processor
    PICO_ELF_EM_ZSP         = 79,  // LSI Logic 16-bit DSP Processor
    PICO_ELF_EM_MMIX        = 80,  // Donald Knuth's educational 64-bit processor
    PICO_ELF_EM_HUANY       = 81,  // Harvard University machine-independent object files
    PICO_ELF_EM_PRISM       = 82,  // SiTera Prism
    PICO_ELF_EM_AVR         = 83,  // Atmel AVR 8-bit microcontroller
    PICO_ELF_EM_FR30        = 84,  // Fujitsu FR30
    PICO_ELF_EM_D10V        = 85,  // Mitsubishi D10V
    PICO_ELF_EM_D30V        = 86,  // Mitsubishi D30V
    PICO_ELF_EM_V850        = 87,  // NEC v850
    PICO_ELF_EM_M32R        = 88,  // Mitsubishi M32R
    PICO_ELF_EM_MN10300     = 89,  // Matsushita MN10300
    PICO_ELF_EM_MN10200     = 90,  // Matsushita MN10200
    PICO_ELF_EM_PJ          = 91,  // picoJava
    PICO_ELF_EM_OPENRISC    = 92,  // OpenRISC 32-bit embedded processor
    PICO_ELF_EM_ARC_A5      = 93,  // ARC Cores Tangent-A5
    PICO_ELF_EM_XTENSA      = 94,  // Tensilica Xtensa Architecture
    PICO_ELF_EM_VIDEOCORE   = 95,  // Alphamosaic VideoCore processor
    PICO_ELF_EM_TMM_GPP     = 96,  // Thompson Multimedia General Purpose Processor
    PICO_ELF_EM_NS32K       = 97,  // National Semiconductor 32000 series
    PICO_ELF_EM_TPC         = 98,  // Tenor Network TPC processor
    PICO_ELF_EM_SNP1K       = 99,  // Trebia SNP 1000 processor
    PICO_ELF_EM_ST200       = 100, // STMicroelectronics ST200 microcontroller
    PICO_ELF_EM_RISCV       = 243, // RISC-V
} picoElfMachine;

typedef struct {
    picoElfIdentifier ident;
    picoElfType type;
    picoElfMachine machine;
    picoElfVersion version;
    uint64_t entryPoint;
    uint64_t programHeaderOffset;
    uint64_t sectionHeaderOffset;
    uint32_t flags;
    size_t headerSize;
    size_t programHeaderEntrySize;
    size_t programHeaderCount;
    size_t sectionHeaderEntrySize;
    size_t sectionHeaderCount;
    size_t sectionHeaderStringTableIndex;
} picoElfHeader;

picoElfResult picoElfParseIdentifier(const picoElfUchar *data, size_t size, picoElfIdentifier *outIdent);
picoElfResult picoElfParseHeader(const picoElfUchar *data, size_t size, picoElfHeader *outHeader);

const char *picoElfResultToString(picoElfResult result);
const char *picoElfClassToString(picoElfClass elfClass);
const char *picoElfDataEncodingToString(picoElfDataEncoding dataEncoding);
const char *picoElfVersionToString(picoElfVersion version);
const char *picoElfOSABIToString(picoElfOSABI osABI);
const char *picoElfTypeToString(picoElfType type);
const char *picoElfMachineToString(picoElfMachine machine);

void picoElfIdentifierDebugPrint(int padding, const picoElfIdentifier *ident);
void picoElfHeaderDebugPrint(int padding, const picoElfHeader *header);

#if defined(PICO_IMPLEMENTATION) && !defined(PICO_ELF_IMPLEMENTATION)
#define PICO_ELF_IMPLEMENTATION
#endif

#ifdef PICO_ELF_IMPLEMENTATION

picoElfResult picoElfParseIdentifier(const picoElfUchar *data, size_t size, picoElfIdentifier *outIdent)
{
    PICO_ASSERT(size >= PICO_ELF_EI_NIDENT);
    PICO_ASSERT(data);
    PICO_ASSERT(outIdent);

    memset(outIdent, 0, sizeof(picoElfIdentifier));

    if (memcmp(data, "\x7F"
                     "ELF",
               4) != 0) {
        return PICO_ELF_RESULT_INVALID_MAGIC;
    }
    memcpy(outIdent->magic, data, 4);

    outIdent->elfClass = (picoElfClass)data[4];
    if (outIdent->elfClass != PICO_ELF_CLASS_32 && outIdent->elfClass != PICO_ELF_CLASS_64 && outIdent->elfClass != PICO_ELF_CLASS_NONE) {
        return PICO_ELF_RESULT_UNSUPPORTED_CLASS;
    }

    outIdent->dataEncoding = (picoElfDataEncoding)data[5];
    if (outIdent->dataEncoding != PICO_ELF_DATA_LSB && outIdent->dataEncoding != PICO_ELF_DATA_MSB && outIdent->dataEncoding != PICO_ELF_DATA_NONE) {
        return PICO_ELF_RESULT_UNSUPPORTED_DATA_ENCODING;
    }

    outIdent->version    = (picoElfVersion)data[6];
    outIdent->osABI      = (picoElfOSABI)data[7];
    outIdent->abiVersion = data[8];

    return PICO_ELF_RESULT_SUCCESS;
}

picoElfResult picoElfParseHeader(const picoElfUchar *data, size_t size, picoElfHeader *outHeader)
{
    PICO_ASSERT(size >= 16);
    PICO_ASSERT(data);
    PICO_ASSERT(outHeader);

    memset(outHeader, 0, sizeof(picoElfHeader));

    picoElfResult result = picoElfParseIdentifier(data, size, &outHeader->ident);
    if (result != PICO_ELF_RESULT_SUCCESS) {
        return result;
    }

    size_t offset = PICO_ELF_EI_NIDENT;

    outHeader->type = (picoElfType)(data[offset] | (data[offset + 1] << 8));
    offset += sizeof(picoElf32Half);

    outHeader->machine = (picoElfMachine)(data[offset] | (data[offset + 1] << 8));
    offset += sizeof(picoElf32Half);

    outHeader->version = (picoElfVersion)(data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24));
    offset += sizeof(picoElf32Word);

    if (outHeader->ident.elfClass == PICO_ELF_CLASS_32) {
        outHeader->entryPoint = (picoElf32Addr)(data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24));
        offset += sizeof(picoElf32Addr);

        outHeader->programHeaderOffset = (picoElf32Off)(data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24));
        offset += sizeof(picoElf32Off);

        outHeader->sectionHeaderOffset = (picoElf32Off)(data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24));
        offset += sizeof(picoElf32Off);
    } else if (outHeader->ident.elfClass == PICO_ELF_CLASS_64) {
        outHeader->entryPoint = (picoElf64Addr)(data[offset] | ((picoElf64Addr)data[offset + 1] << 8) | ((picoElf64Addr)data[offset + 2] << 16) | ((picoElf64Addr)data[offset + 3] << 24) |
                                                ((picoElf64Addr)data[offset + 4] << 32) | ((picoElf64Addr)data[offset + 5] << 40) | ((picoElf64Addr)data[offset + 6] << 48) | ((picoElf64Addr)data[offset + 7] << 56));
        offset += sizeof(picoElf64Addr);

        outHeader->programHeaderOffset = (picoElf64Off)(data[offset] | ((picoElf64Off)data[offset + 1] << 8) | ((picoElf64Off)data[offset + 2] << 16) | ((picoElf64Off)data[offset + 3] << 24) |
                                                        ((picoElf64Off)data[offset + 4] << 32) | ((picoElf64Off)data[offset + 5] << 40) | ((picoElf64Off)data[offset + 6] << 48) | ((picoElf64Off)data[offset + 7] << 56));
        offset += sizeof(picoElf64Off);

        outHeader->sectionHeaderOffset = (picoElf64Off)(data[offset] | ((picoElf64Off)data[offset + 1] << 8) | ((picoElf64Off)data[offset + 2] << 16) | ((picoElf64Off)data[offset + 3] << 24) |
                                                        ((picoElf64Off)data[offset + 4] << 32) | ((picoElf64Off)data[offset + 5] << 40) | ((picoElf64Off)data[offset + 6] << 48) | ((picoElf64Off)data[offset + 7] << 56));
        offset += sizeof(picoElf64Off);
    } else {
        return PICO_ELF_RESULT_UNSUPPORTED_CLASS;
    }

    outHeader->flags = (data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24));
    offset += sizeof(picoElf32Word);

    outHeader->headerSize = (size_t)(data[offset] | (data[offset + 1] << 8));
    offset += sizeof(picoElf32Half);

    outHeader->programHeaderEntrySize = (size_t)(data[offset] | (data[offset + 1] << 8));
    offset += sizeof(picoElf32Half);

    outHeader->programHeaderCount = (size_t)(data[offset] | (data[offset + 1] << 8));
    offset += sizeof(picoElf32Half);

    outHeader->sectionHeaderEntrySize = (size_t)(data[offset] | (data[offset + 1] << 8));
    offset += sizeof(picoElf32Half);

    outHeader->sectionHeaderCount = (size_t)(data[offset] | (data[offset + 1] << 8));
    offset += sizeof(picoElf32Half);

    outHeader->sectionHeaderStringTableIndex = (size_t)(data[offset] | (data[offset + 1] << 8));
    offset += sizeof(picoElf32Half);

    return PICO_ELF_RESULT_SUCCESS;
}

const char *picoElfResultToString(picoElfResult result)
{
    switch (result) {
        case PICO_ELF_RESULT_SUCCESS:
            return "Success";
        case PICO_ELF_RESULT_INVALID_MAGIC:
            return "Invalid magic number";
        case PICO_ELF_RESULT_UNSUPPORTED_CLASS:
            return "Unsupported class";
        case PICO_ELF_RESULT_UNSUPPORTED_DATA_ENCODING:
            return "Unsupported data encoding";
        default:
            return "Unknown result";
    }
}

const char *picoElfClassToString(picoElfClass elfClass)
{
    switch (elfClass) {
        case PICO_ELF_CLASS_NONE:
            return "Invalid class";
        case PICO_ELF_CLASS_32:
            return "32-bit objects";
        case PICO_ELF_CLASS_64:
            return "64-bit objects";
        default:
            return "Unknown class";
    }
}

const char *picoElfDataEncodingToString(picoElfDataEncoding dataEncoding)
{
    switch (dataEncoding) {
        case PICO_ELF_DATA_NONE:
            return "Invalid data encoding";
        case PICO_ELF_DATA_LSB:
            return "Little-endian";
        case PICO_ELF_DATA_MSB:
            return "Big-endian";
        default:
            return "Unknown data encoding";
    }
}

const char *picoElfVersionToString(picoElfVersion version)
{
    switch (version) {
        case PICO_ELF_EV_NONE:
            return "Invalid version";
        case PICO_ELF_EV_CURRENT:
            return "Current version";
        default:
            return "Unknown version";
    }
}

const char *picoElfOSABIToString(picoElfOSABI osABI)
{
    switch (osABI) {
        case PICO_ELF_OSABI_NONE:
            return "System V ABI";
        case PICO_ELF_OSABI_HPUX:
            return "HP-UX";
        case PICO_ELF_OSABI_NETBSD:
            return "NetBSD";
        case PICO_ELF_OSABI_LINUX:
            return "Linux";
        case PICO_ELF_OSABI_SOLARIS:
            return "Solaris";
        case PICO_ELF_OSABI_AIX:
            return "AIX";
        case PICO_ELF_OSABI_IRIX:
            return "IRIX";
        case PICO_ELF_OSABI_FREEBSD:
            return "FreeBSD";
        case PICO_ELF_OSABI_TRU64:
            return "Compaq TRU64 UNIX";
        case PICO_ELF_OSABI_MODESTO:
            return "Novell Modesto";
        case PICO_ELF_OSABI_OPENBSD:
            return "OpenBSD";
        case PICO_ELF_OSABI_OPENVMS:
            return "OpenVMS";
        case PICO_ELF_OSABI_NSK:
            return "Hewlett-Packard Non-Stop Kernel";
        case PICO_ELF_OSABI_ARM_AEABI:
            return "ARM EABI";
        case PICO_ELF_OSABI_ARM:
            return "ARM";
        case PICO_ELF_OSABI_STANDALONE:
            return "Standalone application";
        default:
            return "Unknown OS ABI";
    }
}

const char *picoElfTypeToString(picoElfType type)
{
    switch (type) {
        case PICO_ELF_ET_NONE:
            return "No file type";
        case PICO_ELF_ET_REL:
            return "Relocatable file";
        case PICO_ELF_ET_EXEC:
            return "Executable file";
        case PICO_ELF_ET_DYN:
            return "Shared object file";
        case PICO_ELF_ET_CORE:
            return "Core file";
        default:
            if (type >= PICO_ELF_ET_LOPROC && type <= PICO_ELF_ET_HIPROC) {
                return "Processor-specific";
            }
            return "Unknown type";
    }
}

const char *picoElfMachineToString(picoElfMachine machine)
{
    switch (machine) {
        case PICO_ELF_EM_NONE:
            return "No machine";
        case PICO_ELF_EM_M32:
            return "AT&T WE 32100";
        case PICO_ELF_EM_SPARC:
            return "SPARC";
        case PICO_ELF_EM_386:
            return "Intel 80386";
        case PICO_ELF_EM_68K:
            return "Motorola 68000";
        case PICO_ELF_EM_88K:
            return "Motorola 88000";
        case PICO_ELF_EM_860:
            return "Intel 80860";
        case PICO_ELF_EM_MIPS:
            return "MIPS I Architecture";
        case PICO_ELF_EM_S370:
            return "IBM System/370 Processor";
        case PICO_ELF_EM_MIPS_RS3_LE:
            return "MIPS RS3000 Little-endian";
        case PICO_ELF_EM_PARISC:
            return "Hewlett-Packard PA-RISC";
        case PICO_ELF_EM_VPP500:
            return "Fujitsu VPP500";
        case PICO_ELF_EM_SPARC32PLUS:
            return "Enhanced instruction set SPARC";
        case PICO_ELF_EM_960:
            return "Intel 80960";
        case PICO_ELF_EM_PPC:
            return "PowerPC";
        case PICO_ELF_EM_PPC64:
            return "64-bit PowerPC";
        case PICO_ELF_EM_S390:
            return "IBM System/390 Processor";
        case PICO_ELF_EM_V800:
            return "NEC V800";
        case PICO_ELF_EM_FR20:
            return "Fujitsu FR20";
        case PICO_ELF_EM_RH32:
            return "TRW RH-32";
        case PICO_ELF_EM_RCE:
            return "Motorola RCE";
        case PICO_ELF_EM_ARM:
            return "Advanced RISC Machines ARM";
        case PICO_ELF_EM_ALPHA:
            return "Digital Alpha";
        case PICO_ELF_EM_SH:
            return "Hitachi SH";
        case PICO_ELF_EM_SPARCV9:
            return "SPARC Version 9";
        case PICO_ELF_EM_TRICORE:
            return "Siemens TriCore embedded processor";
        case PICO_ELF_EM_ARC:
            return "Argonaut RISC Core";
        case PICO_ELF_EM_H8_300:
            return "Hitachi H8/300";
        case PICO_ELF_EM_H8_300H:
            return "Hitachi H8/300H";
        case PICO_ELF_EM_H8S:
            return "Hitachi H8S";
        case PICO_ELF_EM_H8_500:
            return "Hitachi H8/500";
        case PICO_ELF_EM_IA_64:
            return "Intel IA-64 processor architecture";
        case PICO_ELF_EM_MIPS_X:
            return "Stanford MIPS-X";
        case PICO_ELF_EM_COLDFIRE:
            return "Motorola ColdFire";
        case PICO_ELF_EM_68HC12:
            return "Motorola M68HC12";
        case PICO_ELF_EM_MMA:
            return "Fujitsu MMA Multimedia Accelerator";
        case PICO_ELF_EM_PCP:
            return "Siemens PCP";
        case PICO_ELF_EM_NCPU:
            return "Sony nCPU embedded RISC processor";
        case PICO_ELF_EM_NDR1:
            return "Denso NDR1 microprocessor";
        case PICO_ELF_EM_STARCORE:
            return "Motorola Star*Core processor";
        case PICO_ELF_EM_ME16:
            return "Toyota ME16 processor";
        case PICO_ELF_EM_ST100:
            return "STMicroelectronics ST100 processor";
        case PICO_ELF_EM_TINYJ:
            return "Advanced Logic Corp. TinyJ embedded processor family";
        case PICO_ELF_EM_X86_64:
            return "AMD x86-64 architecture";
        case PICO_ELF_EM_PDSP:
            return "Sony DSP Processor";
        case PICO_ELF_EM_PDP10:
            return "Digital Equipment Corp. PDP-10";
        case PICO_ELF_EM_PDP11:
            return "Digital Equipment Corp. PDP-11";
        case PICO_ELF_EM_FX66:
            return "Siemens FX66 microcontroller";
        case PICO_ELF_EM_ST9PLUS:
            return "STMicroelectronics ST9+ 8/16 bit microcontroller";
        case PICO_ELF_EM_ST7:
            return "STMicroelectronics ST7 8-bit microcontroller";
        case PICO_ELF_EM_68HC16:
            return "Motorola MC68HC16 Microcontroller";
        case PICO_ELF_EM_68HC11:
            return "Motorola MC68HC11 Microcontroller";
        case PICO_ELF_EM_68HC08:
            return "Motorola MC68HC08 Microcontroller";
        case PICO_ELF_EM_68HC05:
            return "Motorola MC68HC05 Microcontroller";
        case PICO_ELF_EM_SVX:
            return "Silicon Graphics SVx";
        case PICO_ELF_EM_ST19:
            return "STMicroelectronics ST19 8-bit microcontroller";
        case PICO_ELF_EM_VAX:
            return "Digital VAX";
        case PICO_ELF_EM_CRIS:
            return "Axis Communications 32-bit embedded processor";
        case PICO_ELF_EM_JAVELIN:
            return "Infineon Technologies 32-bit embedded processor";
        case PICO_ELF_EM_FIREPATH:
            return "Element 14 64-bit DSP Processor";
        case PICO_ELF_EM_ZSP:
            return "LSI Logic 16-bit DSP Processor";
        case PICO_ELF_EM_MMIX:
            return "Donald Knuth's educational 64-bit processor";
        case PICO_ELF_EM_HUANY:
            return "Harvard University machine-independent object files";
        case PICO_ELF_EM_PRISM:
            return "SiTera Prism";
        case PICO_ELF_EM_AVR:
            return "Atmel AVR 8-bit microcontroller";
        case PICO_ELF_EM_FR30:
            return "Fujitsu FR30";
        case PICO_ELF_EM_D10V:
            return "Mitsubishi D10V";
        case PICO_ELF_EM_D30V:
            return "Mitsubishi D30V";
        case PICO_ELF_EM_V850:
            return "NEC v850";
        case PICO_ELF_EM_M32R:
            return "Mitsubishi M32R";
        case PICO_ELF_EM_MN10300:
            return "Matsushita MN10300";
        case PICO_ELF_EM_MN10200:
            return "Matsushita MN10200";
        case PICO_ELF_EM_PJ:
            return "picoJava";
        case PICO_ELF_EM_OPENRISC:
            return "OpenRISC 32-bit embedded processor";
        case PICO_ELF_EM_ARC_A5:
            return "ARC Cores Tangent-A5";
        case PICO_ELF_EM_XTENSA:
            return "Tensilica Xtensa Architecture";
        case PICO_ELF_EM_VIDEOCORE:
            return "Alphamosaic VideoCore processor";
        case PICO_ELF_EM_TMM_GPP:
            return "Thompson Multimedia General Purpose Processor";
        case PICO_ELF_EM_NS32K:
            return "National Semiconductor 32000 series";
        case PICO_ELF_EM_TPC:
            return "Tenor Network TPC processor";
        case PICO_ELF_EM_SNP1K:
            return "Trebia SNP 1000 processor";
        case PICO_ELF_EM_ST200:
            return "STMicroelectronics ST200 microcontroller";
        case PICO_ELF_EM_RISCV:
            return "RISC-V";
        default:
            return "Unknown machine";
    }
}

void picoElfIdentifierDebugPrint(int padding, const picoElfIdentifier *ident)
{
    if (!ident) {
        PICO_ELF_LOG("%*spicoElfIdentifier: NULL\n", padding, "");
        return;
    }

    PICO_ELF_LOG("%*sELF Identifier\n", padding, "");
    PICO_ELF_LOG("%*sMagic: 0x%02X 0x%02X 0x%02X 0x%02X (", padding, "", ident->magic[0], ident->magic[1], ident->magic[2], ident->magic[3]);
    for (int i = 0; i < 4; i++) {
        if (ident->magic[i] >= 32 && ident->magic[i] < 127) {
            PICO_ELF_LOG("%c", (char)ident->magic[i]);
        } else {
            PICO_ELF_LOG(".");
        }
    }
    PICO_ELF_LOG(")\n");
    PICO_ELF_LOG("%*sClass: %s\n", padding, "", picoElfClassToString(ident->elfClass));
    PICO_ELF_LOG("%*sData Encoding: %s\n", padding, "", picoElfDataEncodingToString(ident->dataEncoding));
    PICO_ELF_LOG("%*sVersion: t%s\n", padding, "", picoElfVersionToString(ident->version));
    PICO_ELF_LOG("%*sOS ABI: %s\n", padding, "", picoElfOSABIToString(ident->osABI));
    PICO_ELF_LOG("%*sABI Version: %u\n", padding, "", ident->abiVersion);
}

void picoElfHeaderDebugPrint(int padding, const picoElfHeader *header)
{
    if (!header) {
        PICO_ELF_LOG("%*spicoElfHeader: NULL\n", padding, "");
        return;
    }

    PICO_ELF_LOG("%*sELF Header\n", padding, "");
    picoElfIdentifierDebugPrint(padding + 1, &header->ident);
    PICO_ELF_LOG("%*sType: %s\n", padding, "", picoElfTypeToString(header->type));
    PICO_ELF_LOG("%*sMachine: %s\n", padding, "", picoElfMachineToString(header->machine));
    PICO_ELF_LOG("%*sVersion: %s\n", padding, "", picoElfVersionToString(header->version));
    PICO_ELF_LOG("%*sEntry Point: 0x%llX\n", padding, "", (unsigned long long)header->entryPoint);
    PICO_ELF_LOG("%*sProgram Header Offset: 0x%llX\n", padding, "", (unsigned long long)header->programHeaderOffset);
    PICO_ELF_LOG("%*sSection Header Offset: 0x%llX\n", padding, "", (unsigned long long)header->sectionHeaderOffset);
    PICO_ELF_LOG("%*sFlags: 0x%08X\n", padding, "", header->flags);
    PICO_ELF_LOG("%*sHeader Size: %zu bytes\n", padding, "", header->headerSize);
    PICO_ELF_LOG("%*sProgram Header Entry Size: %zu bytes\n", padding, "", header->programHeaderEntrySize);
    PICO_ELF_LOG("%*sProgram Header Count: %zu\n", padding, "", header->programHeaderCount);
    PICO_ELF_LOG("%*sSection Header Entry Size: %zu bytes\n", padding, "", header->sectionHeaderEntrySize);
    PICO_ELF_LOG("%*sSection Header Count: %zu\n", padding, "", header->sectionHeaderCount);
    PICO_ELF_LOG("%*sSection Header String Table Index: %zu\n", padding, "", header->sectionHeaderStringTableIndex);
}

#endif // PICO_ELF_IMPLEMENTATION
#endif // PICO_ELF_H
