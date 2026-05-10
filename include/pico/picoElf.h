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
    PICO_ELF_RESULT_OUT_OF_BOUNDS,
    PICO_ELF_RESULT_SYMBOL_TABLE_SECTION_VARIABLE_SIZE,
    PICO_ELF_RESULT_COUNT
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

typedef enum {
    PICO_ELF_SHT_NULL          = 0,          // Section header table entry unused
    PICO_ELF_SHT_PROGBITS      = 1,          // Program data
    PICO_ELF_SHT_SYMTAB        = 2,          // Symbol table
    PICO_ELF_SHT_STRTAB        = 3,          // String table
    PICO_ELF_SHT_RELA          = 4,          // Relocation entries with addends
    PICO_ELF_SHT_HASH          = 5,          // Symbol hash table
    PICO_ELF_SHT_DYNAMIC       = 6,          // Dynamic linking information
    PICO_ELF_SHT_NOTE          = 7,          // Notes
    PICO_ELF_SHT_NOBITS        = 8,          // Program space with no data (bss)
    PICO_ELF_SHT_REL           = 9,          // Relocation entries, no addends
    PICO_ELF_SHT_SHLIB         = 10,         // Reserved
    PICO_ELF_SHT_DYNSYM        = 11,         // Dynamic linker symbol table
    PICO_ELF_SHT_INIT_ARRAY    = 14,         // Array of constructors
    PICO_ELF_SHT_FINI_ARRAY    = 15,         // Array of destructors
    PICO_ELF_SHT_PREINIT_ARRAY = 16,         // Array of pre-constructors
    PICO_ELF_SHT_GROUP         = 17,         // Section group
    PICO_ELF_SHT_SYMTAB_SHNDX  = 18,         // Extended section indices
    PICO_ELF_SHT_LOOS          = 0x60000000, // Start of OS-specific
    PICO_ELF_SHT_HIOS          = 0x6fffffff, // End of OS-specific
    PICO_ELF_SHT_LOPROC        = 0x70000000, // Start of processor-specific
    PICO_ELF_SHT_HIPROC        = 0x7fffffff, // End of processor-specific
    // PICO_ELF_SHT_LOUSER        = 0x80000000, // Start of application-specific
    // PICO_ELF_SHT_HIUSER        = 0xffffffff  // End of application-specific
} picoElfSectionType;

typedef enum {
    PICO_ELF_SHF_WRITE            = 0x1,        // Writable
    PICO_ELF_SHF_ALLOC            = 0x2,        // Occupies memory during execution
    PICO_ELF_SHF_EXECINSTR        = 0x4,        // Executable
    PICO_ELF_SHF_MERGE            = 0x10,       // Might be merged to eliminate duplication
    PICO_ELF_SHF_STRINGS          = 0x20,       // Contains null-terminated strings
    PICO_ELF_SHF_INFO_LINK        = 0x40,       // 'sh_info' contains SHT index
    PICO_ELF_SHF_LINK_ORDER       = 0x80,       // Adds special ordering requirements for link editors
    PICO_ELF_SHF_OS_NONCONFORMING = 0x100,      // OS-specific processing required
    PICO_ELF_SHF_GROUP            = 0x200,      // Section is member of a group
    PICO_ELF_SHF_TLS              = 0x400,      // Section holds Thread-Local Storage
    PICO_ELF_SHF_COMPRESSED       = 0x800,      // Section with compressed data
    // PICO_ELF_SHF_MASKOS           = 0x0ff00000, // OS-specific
    // PICO_ELF_SHF_MASKPROC         = 0xf0000000, // Processor-specific
} picoElfSectionFlags;

typedef struct {
    size_t nameOffset;
    picoElfSectionType type;
    picoElf64Xword flags;
    uint64_t virtualAddress;
    uint64_t fileOffset;
    size_t sectionSize;
    uint32_t link;
    uint32_t info;
    uint64_t addressAlignment;
    size_t entrySize;
} picoElfSectionHeader;

typedef enum {
    // Local symbols are not visible outside the object file 
    // containing their definition. Local symbols of the same name may exist in 
    // multiple files without interfering with each other.
    PICO_ELF_STB_LOCAL  = 0,
    // Global symbols are visible to all object files being combined.
    // One file's definition of a global symbol will satisfy another 
    // file's undefined reference to the same global symbol.
    PICO_ELF_STB_GLOBAL = 1, 
    // Weak symbols resemble global symbols, but their definitions
    // have lower precedence.
    PICO_ELF_STB_WEAK   = 2, 
    // Values in this inclusive range are reserved for operating system-specific semantics.
    PICO_ELF_STB_LOOS   = 10, 
    PICO_ELF_STB_HIOS   = 12,
    // Values in this inclusive range are reserved for processor-specific semantics.
    // If meanings are specified, the processor supplement explains them.
    PICO_ELF_STB_LOPROC = 13, 
    PICO_ELF_STB_HIPROC = 15
} picoElfSymbolBinding;

typedef enum {
    // Symbol type is unspecified
    PICO_ELF_STT_NOTYPE  = 0, 
    // Symbol is a data object, such as a variable, an array, etc.
    PICO_ELF_STT_OBJECT  = 1, 
    // Symbol is executable code, such as a function or a method.
    PICO_ELF_STT_FUNC    = 2, 
    // The symbol is associated with a section. Symbol table entries of this
    // type exist primarily for relocation and normally have STB_LOCAL binding.
    PICO_ELF_STT_SECTION = 3, 
    // Conventionally, the symbol's name gives the name of the source file 
    // associated with the object file. A file symbol has STB_LOCAL binding, 
    // its section index is SHN_ABS, and it precedes the other STB_LOCAL 
    // symbols for the file, if it is present.
    PICO_ELF_STT_FILE    = 4, 
    // An uninitialized common block. 
    PICO_ELF_STT_COMMON  = 5, 
    // The symbol specifies a Thread-Local Storage entity. When defined, it 
    // gives the assigned offset for the symbol, not the actual address. 
    // Symbols of type STT_TLS can be referenced by only special thread-local 
    // storage relocations and thread-local storage relocations can only 
    // reference symbols with type STT_TLS. Implementation need 
    // not support thread-local storage.
    PICO_ELF_STT_TLS     = 6, 
    // Values in this inclusive range are reserved for operating system-specific semantics.
    PICO_ELF_STT_LOOS   = 10, 
    PICO_ELF_STT_HIOS   = 12,
    // Values in this inclusive range are reserved for processor-specific semantics. 
    // If meanings are specified, the processor supplement explains them.
    PICO_ELF_STT_LOPROC = 13, 
    PICO_ELF_STT_HIPROC = 15
} picoElfSymbolType;

typedef enum {
    // The visibility of symbols with the STV_DEFAULT attribute is as specified by 
    // the symbol's binding type. That is, global and weak symbols are visible 
    // outside of their defining component (executable file or shared object). 
    // Local symbols are hidden, as described below. Global and weak symbols 
    // are also preemptable, that is, they may by preempted by definitions of 
    // the same name in another component.
    PICO_ELF_STV_DEFAULT   = 0,
    // A symbol defined in the current component is protected if it is visible 
    // in other components but not preemptable, meaning that any reference to such 
    // a symbol from within the defining component must be resolved to the definition in 
    // that component, even if there is a definition in another component that 
    // would preempt by the default rules. A symbol with STB_LOCAL binding may 
    // not have STV_PROTECTED visibility. If a symbol definition with STV_PROTECTED 
    // visibility from a shared object is taken as resolving a reference from an 
    // executable or another shared object, the SHN_UNDEF symbol table entry 
    // created has STV_DEFAULT visibility.
    PICO_ELF_STV_INTERNAL  = 1,
    // A symbol defined in the current component is hidden if its name is not visible
    // to other components. Such a symbol is necessarily protected. 
    // This attribute may be used to control the external interface of a component. 
    // Note that an object named by such a symbol may still be referenced from 
    // another component if its address is passed outside. A hidden symbol contained 
    // in a relocatable object must be either removed or converted to STB_LOCAL binding 
    // by the link-editor when the relocatable object is included in 
    // an executable file or shared object.
    PICO_ELF_STV_HIDDEN    = 2,
    // The meaning of this visibility attribute may be defined by processor supplements 
    // to further constrain hidden symbols. A processor supplement's definition should be 
    // such that generic tools can safely treat internal symbols as hidden. An internal 
    // symbol contained in a relocatable object must be either removed or converted 
    // to STB_LOCAL binding by the link-editor when the relocatable object is included 
    // in an executable file or shared object.
    PICO_ELF_STV_PROTECTED = 3 
} picoElfSymbolVisibility;

typedef struct {
    size_t nameOffset;
    size_t value;
    size_t size;
    picoElfUchar info;
    picoElfUchar other;
    picoElf32Half sectionIndex;
} picoElfSymbolTableEntry;

typedef struct {
    uint64_t offset;
    uint64_t info;
    int64_t addend;
    bool isRela;
} picoElfRelocationEntry;

typedef enum {
    PICO_ELF_PT_NULL    = 0,          // Program header table entry unused
    PICO_ELF_PT_LOAD    = 1,          // Loadable segment
    PICO_ELF_PT_DYNAMIC = 2,          // Dynamic linking information
    PICO_ELF_PT_INTERP  = 3,          // Interpreter information
    PICO_ELF_PT_NOTE    = 4,          // Auxiliary information
    PICO_ELF_PT_SHLIB   = 5,          // Reserved
    PICO_ELF_PT_PHDR    = 6,          // Entry for header table itself
    PICO_ELF_PT_TLS     = 7,          // Thread-local storage segment
    PICO_ELF_PT_LOOS    = 0x60000000, // Start of OS-specific
    PICO_ELF_PT_HIOS    = 0x6fffffff, // End of OS-specific
    PICO_ELF_PT_LOPROC  = 0x70000000, // Start of processor-specific
    PICO_ELF_PT_HIPROC  = 0x7fffffff  // End of processor-specific
} picoElfProgramType;

typedef enum {
    PICO_ELF_PF_X        = 0x1,        // Execute
    PICO_ELF_PF_W        = 0x2,        // Write
    PICO_ELF_PF_R        = 0x4,        // Read
    PICO_ELF_PF_MASKOS   = 0x0ff00000, // OS-specific
    // PICO_ELF_PF_MASKPROC = 0xf0000000  // Processor-specific
} picoElfProgramFlags;

typedef struct {
    picoElfProgramType type;
    size_t offset;
    size_t virtualAddress;
    size_t physicalAddress;
    size_t fileSize; // maybe 0
    size_t memorySize; // maybe 0
    uint64_t flags;
    size_t alignment;
} picoElfProgramHeader;

picoElfResult picoElfParseIdentifier(const picoElfUchar *data, size_t size, picoElfIdentifier *outIdent);
picoElfResult picoElfParseHeader(const picoElfUchar *data, size_t size, picoElfHeader *outHeader);
picoElfResult picoElfWriteHeader(picoElfUchar *data, size_t size, const picoElfHeader *header);

picoElfResult picoElfParseProgramHeader(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    size_t index,
    picoElfProgramHeader *outProgramHeader);
picoElfResult picoElfWriteProgramHeader(
    picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    size_t index,
    const picoElfProgramHeader *programHeader);
picoElfResult picoElfParseProgramHeaderTable(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    picoElfProgramHeader *outProgramHeaders,
    size_t maxProgramHeaders,
    size_t* outProgramHeaderCount);

picoElfResult picoElfParseSectionHeader(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    size_t index,
    picoElfSectionHeader *outSectionHeader);
picoElfResult picoElfWriteSectionHeader(
    picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    size_t index,
    const picoElfSectionHeader *sectionHeader);
picoElfResult picoElfParseSectionHeaderTable(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    picoElfSectionHeader *outSectionHeaders,
    size_t maxSectionHeaders,
    size_t* outSectionHeaderCount);

picoElfResult picoElfParseSymbolTableEntry(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    const picoElfSectionHeader *sectionHeader,
    size_t index,
    picoElfSymbolTableEntry *outSymbolTableEntry);
picoElfResult picoElfParseSymbolTable(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    const picoElfSectionHeader *sectionHeader,
    picoElfSymbolTableEntry *outSymbolTableEntries,
    size_t maxSymbolTableEntries,
    size_t* outSymbolTableEntryCount);

picoElfResult picoElfParseRelocationEntry(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    const picoElfSectionHeader *sectionHeader,
    size_t index,
    picoElfRelocationEntry *outRelocationEntry);
picoElfResult picoElfParseRelocationTable(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    const picoElfSectionHeader *sectionHeader,
    picoElfRelocationEntry *outRelocationEntries,
    size_t maxRelocationEntries,
    size_t* outRelocationEntryCount);

picoElfSymbolBinding picoElfSymbolTableEntryBinding(const picoElfSymbolTableEntry *entry);
picoElfSymbolType picoElfSymbolTableEntryType(const picoElfSymbolTableEntry *entry);
picoElfSymbolVisibility picoElfSymbolTableEntryVisibility(const picoElfSymbolTableEntry *entry);
picoElfUchar picoElfSymbolTableEntryInfo(picoElfSymbolBinding binding, picoElfSymbolType type);
picoElfUchar picoElfSymbolTableEntryOther(picoElfSymbolVisibility visibility);

uint32_t picoElfRelocationEntrySymbol(picoElfClass elfClass, const picoElfRelocationEntry *entry);
uint32_t picoElfRelocationEntryType(picoElfClass elfClass, const picoElfRelocationEntry *entry);
uint64_t picoElfRelocationEntryInfo(uint32_t symbolIndex, uint32_t type, picoElfClass elfClass);

const char* picoElfStringTableEntry(
    const picoElfUchar* data,
    size_t size,
    const picoElfSectionHeader* stringTableSectionHeader,
    size_t index);

const char *picoElfResultToString(picoElfResult result);
const char *picoElfClassToString(picoElfClass elfClass);
const char *picoElfDataEncodingToString(picoElfDataEncoding dataEncoding);
const char *picoElfVersionToString(picoElfVersion version);
const char *picoElfOSABIToString(picoElfOSABI osABI);
const char *picoElfTypeToString(picoElfType type);
const char *picoElfMachineToString(picoElfMachine machine);
const char *picoElfSectionTypeToString(picoElfSectionType sectionType);
const char *picoElfSectionFlagsToString(picoElfSectionFlags flags);
const char *picoElfSymbolBindingToString(picoElfSymbolBinding binding);
const char *picoElfSymbolTypeToString(picoElfSymbolType type);
const char *picoElfSymbolVisibilityToString(picoElfSymbolVisibility visibility);
const char *picoElfProgramTypeToString(picoElfProgramType type);
const char *picoElfProgramFlagsToString(uint64_t flags);

void picoElfIdentifierDebugPrint(int padding, const picoElfIdentifier *ident);
void picoElfHeaderDebugPrint(int padding, const picoElfHeader *header);
void picoElfProgramHeaderDebugPrint(int padding, const picoElfProgramHeader *programHeader);
void picoElfSectionHeaderDebugPrint(int padding, const picoElfSectionHeader *sectionHeader);
void picoElfSymbolTableEntryDebugPrint(int padding, const picoElfSymbolTableEntry *symbolTableEntry);
void picoElfRelocationEntryDebugPrint(int padding, const picoElfRelocationEntry *relocationEntry);

#if defined(PICO_IMPLEMENTATION) && !defined(PICO_ELF_IMPLEMENTATION)
#define PICO_ELF_IMPLEMENTATION
#endif

#ifdef PICO_ELF_IMPLEMENTATION

#define PICO_ELF__PARSE_U64(dst, buffer, offset) \
    do { \
        (dst) = ((uint64_t)(buffer)[(offset)] | ((uint64_t)(buffer)[(offset) + 1] << 8) | ((uint64_t)(buffer)[(offset) + 2] << 16) | ((uint64_t)(buffer)[(offset) + 3] << 24) | \
                 ((uint64_t)(buffer)[(offset) + 4] << 32) | ((uint64_t)(buffer)[(offset) + 5] << 40) | ((uint64_t)(buffer)[(offset) + 6] << 48) | ((uint64_t)(buffer)[(offset) + 7] << 56)); \
        (offset) += sizeof(uint64_t); \
    } while (0)
#define PICO_ELF__PARSE_S64(dst, buffer, offset) \
    do { \
        (dst) = ((int64_t)(buffer)[(offset)] | ((int64_t)(buffer)[(offset) + 1] << 8) | ((int64_t)(buffer)[(offset) + 2] << 16) | ((int64_t)(buffer)[(offset) + 3] << 24) | \
                 ((int64_t)(buffer)[(offset) + 4] << 32) | ((int64_t)(buffer)[(offset) + 5] << 40) | ((int64_t)(buffer)[(offset) + 6] << 48) | ((int64_t)(buffer)[(offset) + 7] << 56)); \
        (offset) += sizeof(int64_t); \
    } while (0)
#define PICO_ELF__PARSE_U32(dst, buffer, offset) \
    do { \
        (dst) = ((uint32_t)(buffer)[(offset)] | ((uint32_t)(buffer)[(offset) + 1] << 8) | ((uint32_t)(buffer)[(offset) + 2] << 16) | ((uint32_t)(buffer)[(offset) + 3] << 24)); \
        (offset) += sizeof(uint32_t); \
    } while (0)
#define PICO_ELF__PARSE_S32(dst, buffer, offset) \
    do { \
        (dst) = ((int32_t)(buffer)[(offset)] | ((int32_t)(buffer)[(offset) + 1] << 8) | ((int32_t)(buffer)[(offset) + 2] << 16) | ((int32_t)(buffer)[(offset) + 3] << 24)); \
        (offset) += sizeof(int32_t); \
    } while (0)
#define PICO_ELF__PARSE_U16(dst, buffer, offset) \
    do { \
        (dst) = ((uint16_t)(buffer)[(offset)] | ((uint16_t)(buffer)[(offset) + 1] << 8)); \
        (offset) += sizeof(uint16_t); \
    } while (0)
#define PICO_ELF__PARSE_U8(dst, buffer, offset) \
    do { \
        (dst) = (buffer)[(offset)]; \
        (offset) += sizeof(uint8_t); \
    } while (0)

#define PICO_ELF__WRITE_U64(src, buffer, offset) \
    do { \
        (buffer)[(offset)]     = (picoElfUchar)(((uint64_t)(src)) & 0xFF); \
        (buffer)[(offset) + 1] = (picoElfUchar)((((uint64_t)(src)) >> 8) & 0xFF); \
        (buffer)[(offset) + 2] = (picoElfUchar)((((uint64_t)(src)) >> 16) & 0xFF); \
        (buffer)[(offset) + 3] = (picoElfUchar)((((uint64_t)(src)) >> 24) & 0xFF); \
        (buffer)[(offset) + 4] = (picoElfUchar)((((uint64_t)(src)) >> 32) & 0xFF); \
        (buffer)[(offset) + 5] = (picoElfUchar)((((uint64_t)(src)) >> 40) & 0xFF); \
        (buffer)[(offset) + 6] = (picoElfUchar)((((uint64_t)(src)) >> 48) & 0xFF); \
        (buffer)[(offset) + 7] = (picoElfUchar)((((uint64_t)(src)) >> 56) & 0xFF); \
        (offset) += sizeof(uint64_t); \
    } while (0)

#define PICO_ELF__WRITE_S64(src, buffer, offset) \
    do { \
        (buffer)[(offset)]     = (picoElfUchar)(((uint64_t)(src)) & 0xFF); \
        (buffer)[(offset) + 1] = (picoElfUchar)((((uint64_t)(src)) >> 8) & 0xFF); \
        (buffer)[(offset) + 2] = (picoElfUchar)((((uint64_t)(src)) >> 16) & 0xFF); \
        (buffer)[(offset) + 3] = (picoElfUchar)((((uint64_t)(src)) >> 24) & 0xFF); \
        (buffer)[(offset) + 4] = (picoElfUchar)((((uint64_t)(src)) >> 32) & 0xFF); \
        (buffer)[(offset) + 5] = (picoElfUchar)((((uint64_t)(src)) >> 40) & 0xFF); \
        (buffer)[(offset) + 6] = (picoElfUchar)((((uint64_t)(src)) >> 48) & 0xFF); \
        (buffer)[(offset) + 7] = (picoElfUchar)((((uint64_t)(src)) >> 56) & 0xFF); \
        (offset) += sizeof(int64_t); \
    } while (0)

#define PICO_ELF__WRITE_U32(src, buffer, offset) \
    do { \
        (buffer)[(offset)]     = (picoElfUchar)(((uint32_t)(src)) & 0xFF); \
        (buffer)[(offset) + 1] = (picoElfUchar)((((uint32_t)(src)) >> 8) & 0xFF); \
        (buffer)[(offset) + 2] = (picoElfUchar)((((uint32_t)(src)) >> 16) & 0xFF); \
        (buffer)[(offset) + 3] = (picoElfUchar)((((uint32_t)(src)) >> 24) & 0xFF); \
        (offset) += sizeof(uint32_t); \
    } while (0)

#define PICO_ELF__WRITE_S32(src, buffer, offset) \
    do { \
        (buffer)[(offset)]     = (picoElfUchar)(((uint32_t)(src)) & 0xFF); \
        (buffer)[(offset) + 1] = (picoElfUchar)((((uint32_t)(src)) >> 8) & 0xFF); \
        (buffer)[(offset) + 2] = (picoElfUchar)((((uint32_t)(src)) >> 16) & 0xFF); \
        (buffer)[(offset) + 3] = (picoElfUchar)((((uint32_t)(src)) >> 24) & 0xFF); \
        (offset) += sizeof(int32_t); \
    } while (0)

#define PICO_ELF__WRITE_U16(src, buffer, offset) \
    do { \
        (buffer)[(offset)]     = (picoElfUchar)(((uint16_t)(src)) & 0xFF); \
        (buffer)[(offset) + 1] = (picoElfUchar)((((uint16_t)(src)) >> 8) & 0xFF); \
        (offset) += sizeof(uint16_t); \
    } while (0)

#define PICO_ELF__WRITE_S16(src, buffer, offset) \
    do { \
        (buffer)[(offset)]     = (picoElfUchar)(((uint16_t)(src)) & 0xFF); \
        (buffer)[(offset) + 1] = (picoElfUchar)((((uint16_t)(src)) >> 8) & 0xFF); \
        (offset) += sizeof(int16_t); \
    } while (0)

#define PICO_ELF__WRITE_U8(src, buffer, offset) \
    do { \
        (buffer)[(offset)] = (picoElfUchar)(((uint8_t)(src)) & 0xFF); \
        (offset) += sizeof(uint8_t); \
    } while (0)

#define PICO_ELF__WRITE_S8(src, buffer, offset) \
    do { \
        (buffer)[(offset)] = (picoElfUchar)(((uint8_t)(src)) & 0xFF); \
        (offset) += sizeof(int8_t); \
    } while (0)



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

    PICO_ELF__PARSE_U16(outHeader->type, data, offset);
    PICO_ELF__PARSE_U16(outHeader->machine, data, offset);
    PICO_ELF__PARSE_U32(outHeader->version, data, offset);

    if (outHeader->ident.elfClass == PICO_ELF_CLASS_32) {
        PICO_ELF__PARSE_U32(outHeader->entryPoint, data, offset);
        PICO_ELF__PARSE_U32(outHeader->programHeaderOffset, data, offset);
        PICO_ELF__PARSE_U32(outHeader->sectionHeaderOffset, data, offset);
    } else if (outHeader->ident.elfClass == PICO_ELF_CLASS_64) {
        PICO_ELF__PARSE_U64(outHeader->entryPoint, data, offset);
        PICO_ELF__PARSE_U64(outHeader->programHeaderOffset, data, offset);
        PICO_ELF__PARSE_U64(outHeader->sectionHeaderOffset, data, offset);
    } else {
        return PICO_ELF_RESULT_UNSUPPORTED_CLASS;
    }

    PICO_ELF__PARSE_U32(outHeader->flags, data, offset);
    PICO_ELF__PARSE_U16(outHeader->headerSize, data, offset);
    PICO_ELF__PARSE_U16(outHeader->programHeaderEntrySize, data, offset);
    PICO_ELF__PARSE_U16(outHeader->programHeaderCount, data, offset);
    PICO_ELF__PARSE_U16(outHeader->sectionHeaderEntrySize, data, offset);
    PICO_ELF__PARSE_U16(outHeader->sectionHeaderCount, data, offset);
    PICO_ELF__PARSE_U16(outHeader->sectionHeaderStringTableIndex, data, offset);

    return PICO_ELF_RESULT_SUCCESS;
}

picoElfResult picoElfParseProgramHeader(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    size_t index,
    picoElfProgramHeader *outProgramHeader)
{
    PICO_ASSERT(data);
    PICO_ASSERT(header);
    PICO_ASSERT(outProgramHeader);
    PICO_ASSERT(index < header->programHeaderCount);

    const size_t programHeaderOffset = header->programHeaderOffset + index * header->programHeaderEntrySize;
    if (programHeaderOffset + header->programHeaderEntrySize > size) {
        return PICO_ELF_RESULT_OUT_OF_BOUNDS;
    }

    const picoElfUchar *programHeaderData = data + programHeaderOffset;

    memset(outProgramHeader, 0, sizeof(picoElfProgramHeader));

    size_t offset = 0;
    PICO_ELF__PARSE_U32(outProgramHeader->type, programHeaderData, offset);
    
    if (header->ident.elfClass == PICO_ELF_CLASS_32) {
        PICO_ELF__PARSE_U32(outProgramHeader->offset, programHeaderData, offset);
        PICO_ELF__PARSE_U32(outProgramHeader->virtualAddress, programHeaderData, offset);
        PICO_ELF__PARSE_U32(outProgramHeader->physicalAddress, programHeaderData, offset);
        PICO_ELF__PARSE_U32(outProgramHeader->fileSize, programHeaderData, offset);
        PICO_ELF__PARSE_U32(outProgramHeader->memorySize, programHeaderData, offset);
        PICO_ELF__PARSE_U32(outProgramHeader->flags, programHeaderData, offset);
        PICO_ELF__PARSE_U32(outProgramHeader->alignment, programHeaderData, offset);
    } else if (header->ident.elfClass == PICO_ELF_CLASS_64) {
        PICO_ELF__PARSE_U32(outProgramHeader->flags, programHeaderData, offset);
        PICO_ELF__PARSE_U64(outProgramHeader->offset, programHeaderData, offset);
        PICO_ELF__PARSE_U64(outProgramHeader->virtualAddress, programHeaderData, offset);
        PICO_ELF__PARSE_U64(outProgramHeader->physicalAddress, programHeaderData, offset);
        PICO_ELF__PARSE_U64(outProgramHeader->fileSize, programHeaderData, offset);
        PICO_ELF__PARSE_U64(outProgramHeader->memorySize, programHeaderData, offset);
        PICO_ELF__PARSE_U64(outProgramHeader->alignment, programHeaderData, offset);        
    } else {
        return PICO_ELF_RESULT_UNSUPPORTED_CLASS;
    }

    return PICO_ELF_RESULT_SUCCESS;
}

picoElfResult picoElfParseProgramHeaderTable(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    picoElfProgramHeader *outProgramHeaders,
    size_t maxProgramHeaders,
    size_t* outProgramHeaderCount)
{
    PICO_ASSERT(data);
    PICO_ASSERT(header);
    PICO_ASSERT(outProgramHeaders);
    PICO_ASSERT(outProgramHeaderCount);

    *outProgramHeaderCount = header->programHeaderCount < maxProgramHeaders ? header->programHeaderCount : maxProgramHeaders;

    for (size_t i = 0; i < *outProgramHeaderCount; ++i) {
        picoElfResult result = picoElfParseProgramHeader(data, size, header, i, &outProgramHeaders[i]);
        if (result != PICO_ELF_RESULT_SUCCESS) {
            return result;
        }
    }

    return PICO_ELF_RESULT_SUCCESS;
}

picoElfResult picoElfParseSectionHeader(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    size_t index,
    picoElfSectionHeader *outSectionHeader)
{
    PICO_ASSERT(data);
    PICO_ASSERT(header);
    PICO_ASSERT(outSectionHeader);
    PICO_ASSERT(index < header->sectionHeaderCount);

    const size_t sectionHeaderOffset = header->sectionHeaderOffset + index * header->sectionHeaderEntrySize;
    if (sectionHeaderOffset + header->sectionHeaderEntrySize > size) {
        return PICO_ELF_RESULT_OUT_OF_BOUNDS;
    }

    const picoElfUchar *sectionHeaderData = data + sectionHeaderOffset;

    memset(outSectionHeader, 0, sizeof(picoElfSectionHeader));

    size_t offset = 0;

    PICO_ELF__PARSE_U32(outSectionHeader->nameOffset, sectionHeaderData, offset);
    PICO_ELF__PARSE_U32(outSectionHeader->type, sectionHeaderData, offset);

    if (header->ident.elfClass == PICO_ELF_CLASS_32) {
        PICO_ELF__PARSE_U32(outSectionHeader->flags, sectionHeaderData, offset);
        PICO_ELF__PARSE_U32(outSectionHeader->virtualAddress, sectionHeaderData, offset);
        PICO_ELF__PARSE_U32(outSectionHeader->fileOffset, sectionHeaderData, offset);
        PICO_ELF__PARSE_U32(outSectionHeader->sectionSize, sectionHeaderData, offset);
    } else if (header->ident.elfClass == PICO_ELF_CLASS_64) {
        PICO_ELF__PARSE_U64(outSectionHeader->flags, sectionHeaderData, offset);
        PICO_ELF__PARSE_U64(outSectionHeader->virtualAddress, sectionHeaderData, offset);
        PICO_ELF__PARSE_U64(outSectionHeader->fileOffset, sectionHeaderData, offset);
        PICO_ELF__PARSE_U64(outSectionHeader->sectionSize, sectionHeaderData, offset);
    } else {
        return PICO_ELF_RESULT_UNSUPPORTED_CLASS;
    }

    PICO_ELF__PARSE_U32(outSectionHeader->link, sectionHeaderData, offset);
    PICO_ELF__PARSE_U32(outSectionHeader->info, sectionHeaderData, offset);

    if (header->ident.elfClass == PICO_ELF_CLASS_32) {
        PICO_ELF__PARSE_U32(outSectionHeader->addressAlignment, sectionHeaderData, offset);
        PICO_ELF__PARSE_U32(outSectionHeader->entrySize, sectionHeaderData, offset);
    } else if (header->ident.elfClass == PICO_ELF_CLASS_64) {
        PICO_ELF__PARSE_U64(outSectionHeader->addressAlignment, sectionHeaderData, offset);
        PICO_ELF__PARSE_U64(outSectionHeader->entrySize, sectionHeaderData, offset);
    } else {
        return PICO_ELF_RESULT_UNSUPPORTED_CLASS;
    }

    return PICO_ELF_RESULT_SUCCESS;
}

picoElfResult picoElfParseSectionHeaderTable(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    picoElfSectionHeader *outSectionHeaders,
    size_t maxSectionHeaders,
    size_t* outSectionHeaderCount)
{
    PICO_ASSERT(data);
    PICO_ASSERT(header);
    PICO_ASSERT(outSectionHeaders);
    
    size_t numHeaders = header->sectionHeaderCount > maxSectionHeaders ? maxSectionHeaders : header->sectionHeaderCount;
    if (outSectionHeaderCount) {
        *outSectionHeaderCount = numHeaders;
    }

    memset(outSectionHeaders, 0, sizeof(picoElfSectionHeader) * numHeaders);

    picoElfResult result = PICO_ELF_RESULT_SUCCESS;
    for (size_t i = 0; i < numHeaders; ++i) {
        result = picoElfParseSectionHeader(data, size, header, i, &outSectionHeaders[i]);
        if (result != PICO_ELF_RESULT_SUCCESS) {
            return result;
        }
    }

    return PICO_ELF_RESULT_SUCCESS;
}

picoElfResult picoElfParseSymbolTableEntry(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    const picoElfSectionHeader *sectionHeader,
    size_t index,
    picoElfSymbolTableEntry *outSymbolTableEntry)
{
    PICO_ASSERT(data);
    PICO_ASSERT(header);
    PICO_ASSERT(sectionHeader);
    PICO_ASSERT(outSymbolTableEntry);
    PICO_ASSERT(sectionHeader->type == PICO_ELF_SHT_SYMTAB || sectionHeader->type == PICO_ELF_SHT_DYNSYM);

    const size_t symbolTableEntryOffset = sectionHeader->fileOffset + index * sectionHeader->entrySize;
    if (symbolTableEntryOffset + sectionHeader->entrySize > size) {
        return PICO_ELF_RESULT_OUT_OF_BOUNDS;
    }

    const picoElfUchar *symbolTableEntryData = data + symbolTableEntryOffset;
    memset(outSymbolTableEntry, 0, sizeof(picoElfSymbolTableEntry));

    size_t offset = 0;
    if (header->ident.elfClass == PICO_ELF_CLASS_32) {
        PICO_ELF__PARSE_U32(outSymbolTableEntry->nameOffset, symbolTableEntryData, offset);
        PICO_ELF__PARSE_U32(outSymbolTableEntry->value, symbolTableEntryData, offset);
        PICO_ELF__PARSE_U32(outSymbolTableEntry->size, symbolTableEntryData, offset);
        PICO_ELF__PARSE_U8(outSymbolTableEntry->info, symbolTableEntryData, offset);
        PICO_ELF__PARSE_U8(outSymbolTableEntry->other, symbolTableEntryData, offset);
        PICO_ELF__PARSE_U16(outSymbolTableEntry->sectionIndex, symbolTableEntryData, offset);
    } else if (header->ident.elfClass == PICO_ELF_CLASS_64) {
        PICO_ELF__PARSE_U32(outSymbolTableEntry->nameOffset, symbolTableEntryData, offset);
        PICO_ELF__PARSE_U8(outSymbolTableEntry->info, symbolTableEntryData, offset);
        PICO_ELF__PARSE_U8(outSymbolTableEntry->other, symbolTableEntryData, offset);
        PICO_ELF__PARSE_U16(outSymbolTableEntry->sectionIndex, symbolTableEntryData, offset);
        PICO_ELF__PARSE_U64(outSymbolTableEntry->value, symbolTableEntryData, offset);
        PICO_ELF__PARSE_U64(outSymbolTableEntry->size, symbolTableEntryData, offset);
    } else {
        return PICO_ELF_RESULT_UNSUPPORTED_CLASS;
    }

    return PICO_ELF_RESULT_SUCCESS;
}

picoElfResult picoElfParseSymbolTable(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    const picoElfSectionHeader *sectionHeader,
    picoElfSymbolTableEntry *outSymbolTableEntries,
    size_t maxSymbolTableEntries,
    size_t* outSymbolTableEntryCount)
{
    PICO_ASSERT(data);
    PICO_ASSERT(header);
    PICO_ASSERT(sectionHeader);
    PICO_ASSERT(outSymbolTableEntries);
    
    if (sectionHeader->entrySize == 0) {
        // section entries cant be of variable size
        return PICO_ELF_RESULT_SYMBOL_TABLE_SECTION_VARIABLE_SIZE;
    }

    size_t numEntries = sectionHeader->sectionSize / sectionHeader->entrySize;
    if (numEntries > maxSymbolTableEntries) {
        numEntries = maxSymbolTableEntries;
    }
    if (outSymbolTableEntryCount) {
        *outSymbolTableEntryCount = numEntries;
    }

    memset(outSymbolTableEntries, 0, sizeof(picoElfSymbolTableEntry) * numEntries);

    picoElfResult result = PICO_ELF_RESULT_SUCCESS;
    for (size_t i = 0; i < numEntries; ++i) {
        result = picoElfParseSymbolTableEntry(data, size, header, sectionHeader, i, &outSymbolTableEntries[i]);
        if (result != PICO_ELF_RESULT_SUCCESS) {
            return result;
        }
    }

    return PICO_ELF_RESULT_SUCCESS;
}

picoElfResult picoElfParseRelocationEntry(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    const picoElfSectionHeader *sectionHeader,
    size_t index,
    picoElfRelocationEntry *outRelocationEntry)
{
    PICO_ASSERT(data);
    PICO_ASSERT(header);
    PICO_ASSERT(sectionHeader);
    PICO_ASSERT(outRelocationEntry);
    PICO_ASSERT(sectionHeader->type == PICO_ELF_SHT_REL || sectionHeader->type == PICO_ELF_SHT_RELA);

    const size_t entryOffset = sectionHeader->fileOffset + index * sectionHeader->entrySize;
    if (entryOffset + sectionHeader->entrySize > size) {
        return PICO_ELF_RESULT_OUT_OF_BOUNDS;
    }

    const picoElfUchar *entryData = data + entryOffset;
    memset(outRelocationEntry, 0, sizeof(picoElfRelocationEntry));
    outRelocationEntry->isRela = (sectionHeader->type == PICO_ELF_SHT_RELA);

    size_t offset = 0;
    if (header->ident.elfClass == PICO_ELF_CLASS_32) {
        PICO_ELF__PARSE_U32(outRelocationEntry->offset, entryData, offset);
        PICO_ELF__PARSE_U32(outRelocationEntry->info, entryData, offset);
        if (outRelocationEntry->isRela) {
            PICO_ELF__PARSE_S32(outRelocationEntry->addend, entryData, offset);
        }
    } else if (header->ident.elfClass == PICO_ELF_CLASS_64) {
        PICO_ELF__PARSE_U64(outRelocationEntry->offset, entryData, offset);
        PICO_ELF__PARSE_U64(outRelocationEntry->info, entryData, offset);
        if (outRelocationEntry->isRela) {
            PICO_ELF__PARSE_S64(outRelocationEntry->addend, entryData, offset);
        }
    } else {
        return PICO_ELF_RESULT_UNSUPPORTED_CLASS;
    }

    return PICO_ELF_RESULT_SUCCESS;
}

picoElfResult picoElfParseRelocationTable(
    const picoElfUchar *data,
    size_t size,
    const picoElfHeader *header,
    const picoElfSectionHeader *sectionHeader,
    picoElfRelocationEntry *outRelocationEntries,
    size_t maxRelocationEntries,
    size_t* outRelocationEntryCount)
{
    PICO_ASSERT(data);
    PICO_ASSERT(header);
    PICO_ASSERT(sectionHeader);
    PICO_ASSERT(outRelocationEntries);
    
    if (sectionHeader->entrySize == 0) {
        return PICO_ELF_RESULT_SYMBOL_TABLE_SECTION_VARIABLE_SIZE;
    }

    size_t numEntries = sectionHeader->sectionSize / sectionHeader->entrySize;
    if (numEntries > maxRelocationEntries) {
        numEntries = maxRelocationEntries;
    }
    if (outRelocationEntryCount) {
        *outRelocationEntryCount = numEntries;
    }

    memset(outRelocationEntries, 0, sizeof(picoElfRelocationEntry) * numEntries);

    picoElfResult result = PICO_ELF_RESULT_SUCCESS;
    for (size_t i = 0; i < numEntries; ++i) {
        result = picoElfParseRelocationEntry(data, size, header, sectionHeader, i, &outRelocationEntries[i]);
        if (result != PICO_ELF_RESULT_SUCCESS) {
            return result;
        }
    }

    return PICO_ELF_RESULT_SUCCESS;
}

picoElfResult picoElfWriteHeader(
    picoElfUchar *buffer,
    size_t size,
    const picoElfHeader *header)
{
    PICO_ASSERT(buffer);
    PICO_ASSERT(header);

    if (size < PICO_ELF_EI_NIDENT) {
        return PICO_ELF_RESULT_OUT_OF_BOUNDS;
    }

    size_t offset = 0;

    buffer[0] = 0x7f;
    buffer[1] = 'E';
    buffer[2] = 'L';
    buffer[3] = 'F';
    offset += 4;

    PICO_ELF__WRITE_U8(header->ident.elfClass, buffer, offset);
    PICO_ELF__WRITE_U8(header->ident.dataEncoding, buffer, offset);
    PICO_ELF__WRITE_U8(header->ident.version, buffer, offset);
    PICO_ELF__WRITE_U8(header->ident.osABI, buffer, offset);
    PICO_ELF__WRITE_U8(header->ident.abiVersion, buffer, offset);

    for (int i = 0; i < 7; ++i) {
        PICO_ELF__WRITE_U8(0, buffer, offset);
    }

    PICO_ELF__WRITE_U16(header->type, buffer, offset);
    PICO_ELF__WRITE_U16(header->machine, buffer, offset);
    PICO_ELF__WRITE_U32(header->version, buffer, offset);

    if (header->ident.elfClass == PICO_ELF_CLASS_32) {
        PICO_ELF__WRITE_U32(header->entryPoint, buffer, offset);
        PICO_ELF__WRITE_U32(header->programHeaderOffset, buffer, offset);
        PICO_ELF__WRITE_U32(header->sectionHeaderOffset, buffer, offset);
    } else if (header->ident.elfClass == PICO_ELF_CLASS_64) {
        PICO_ELF__WRITE_U64(header->entryPoint, buffer, offset);
        PICO_ELF__WRITE_U64(header->programHeaderOffset, buffer, offset);
        PICO_ELF__WRITE_U64(header->sectionHeaderOffset, buffer, offset);
    } else {
        return PICO_ELF_RESULT_UNSUPPORTED_CLASS;
    }

    PICO_ELF__WRITE_U32(header->flags, buffer, offset);
    PICO_ELF__WRITE_U16(header->headerSize, buffer, offset);
    PICO_ELF__WRITE_U16(header->programHeaderEntrySize, buffer, offset);
    PICO_ELF__WRITE_U16(header->programHeaderCount, buffer, offset);
    PICO_ELF__WRITE_U16(header->sectionHeaderEntrySize, buffer, offset);
    PICO_ELF__WRITE_U16(header->sectionHeaderCount, buffer, offset);
    PICO_ELF__WRITE_U16(header->sectionHeaderStringTableIndex, buffer, offset);

    return PICO_ELF_RESULT_SUCCESS;
}

picoElfResult picoElfWriteProgramHeader(
    picoElfUchar *buffer,
    size_t size,
    const picoElfHeader *header,
    size_t index,
    const picoElfProgramHeader *programHeader)
{
    PICO_ASSERT(buffer);
    PICO_ASSERT(header);
    PICO_ASSERT(programHeader);

    size_t offset = header->programHeaderOffset + index * header->programHeaderEntrySize;

    if (offset + header->programHeaderEntrySize > size) {
        return PICO_ELF_RESULT_OUT_OF_BOUNDS;
    }

    PICO_ELF__WRITE_U32(programHeader->type, buffer, offset);

    if (header->ident.elfClass == PICO_ELF_CLASS_32) {
        PICO_ELF__WRITE_U32(programHeader->offset, buffer, offset);
        PICO_ELF__WRITE_U32(programHeader->virtualAddress, buffer, offset);
        PICO_ELF__WRITE_U32(programHeader->physicalAddress, buffer, offset);
        PICO_ELF__WRITE_U32(programHeader->fileSize, buffer, offset);
        PICO_ELF__WRITE_U32(programHeader->memorySize, buffer, offset);
        PICO_ELF__WRITE_U32(programHeader->flags, buffer, offset);
        PICO_ELF__WRITE_U32(programHeader->alignment, buffer, offset);
    } else if (header->ident.elfClass == PICO_ELF_CLASS_64) {
        PICO_ELF__WRITE_U32(programHeader->flags, buffer, offset);
        PICO_ELF__WRITE_U64(programHeader->offset, buffer, offset);
        PICO_ELF__WRITE_U64(programHeader->virtualAddress, buffer, offset);
        PICO_ELF__WRITE_U64(programHeader->physicalAddress, buffer, offset);
        PICO_ELF__WRITE_U64(programHeader->fileSize, buffer, offset);
        PICO_ELF__WRITE_U64(programHeader->memorySize, buffer, offset);
        PICO_ELF__WRITE_U64(programHeader->alignment, buffer, offset);
    } else {
        return PICO_ELF_RESULT_UNSUPPORTED_CLASS;
    }

    return PICO_ELF_RESULT_SUCCESS;
}

picoElfResult picoElfWriteSectionHeader(
    picoElfUchar *buffer,
    size_t size,
    const picoElfHeader *header,
    size_t index,
    const picoElfSectionHeader *sectionHeader)
{
    PICO_ASSERT(buffer);
    PICO_ASSERT(header);
    PICO_ASSERT(sectionHeader);

    size_t offset = header->sectionHeaderOffset + index * header->sectionHeaderEntrySize;

    if (offset + header->sectionHeaderEntrySize > size) {
        return PICO_ELF_RESULT_OUT_OF_BOUNDS;
    }

    PICO_ELF__WRITE_U32(sectionHeader->nameOffset, buffer, offset);
    PICO_ELF__WRITE_U32(sectionHeader->type, buffer, offset);

    if (header->ident.elfClass == PICO_ELF_CLASS_32) {
        PICO_ELF__WRITE_U32(sectionHeader->flags, buffer, offset);
        PICO_ELF__WRITE_U32(sectionHeader->virtualAddress, buffer, offset);
        PICO_ELF__WRITE_U32(sectionHeader->fileOffset, buffer, offset);
        PICO_ELF__WRITE_U32(sectionHeader->sectionSize, buffer, offset);
    } else if (header->ident.elfClass == PICO_ELF_CLASS_64) {
        PICO_ELF__WRITE_U64(sectionHeader->flags, buffer, offset);
        PICO_ELF__WRITE_U64(sectionHeader->virtualAddress, buffer, offset);
        PICO_ELF__WRITE_U64(sectionHeader->fileOffset, buffer, offset);
        PICO_ELF__WRITE_U64(sectionHeader->sectionSize, buffer, offset);
    } else {
        return PICO_ELF_RESULT_UNSUPPORTED_CLASS;
    }

    PICO_ELF__WRITE_U32(sectionHeader->link, buffer, offset);
    PICO_ELF__WRITE_U32(sectionHeader->info, buffer, offset);

    if (header->ident.elfClass == PICO_ELF_CLASS_32) {
        PICO_ELF__WRITE_U32(sectionHeader->addressAlignment, buffer, offset);
        PICO_ELF__WRITE_U32(sectionHeader->entrySize, buffer, offset);
    } else if (header->ident.elfClass == PICO_ELF_CLASS_64) {
        PICO_ELF__WRITE_U64(sectionHeader->addressAlignment, buffer, offset);
        PICO_ELF__WRITE_U64(sectionHeader->entrySize, buffer, offset);
    } else {
        return PICO_ELF_RESULT_UNSUPPORTED_CLASS;
    }

    return PICO_ELF_RESULT_SUCCESS;
}

// based on https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.symtab.html
picoElfSymbolBinding picoElfSymbolTableEntryBinding(const picoElfSymbolTableEntry *entry)
{
    return (picoElfSymbolBinding)(entry->info >> 4);
}

// based on https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.symtab.html
picoElfSymbolType picoElfSymbolTableEntryType(const picoElfSymbolTableEntry *entry)
{
    return (picoElfSymbolType)(entry->info & 0xF);
}

// based on https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.symtab.html
picoElfSymbolVisibility picoElfSymbolTableEntryVisibility(const picoElfSymbolTableEntry *entry)
{
    return (picoElfSymbolVisibility)(entry->other & 0x3);
}

// based on https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.symtab.html
picoElfUchar picoElfSymbolTableEntryInfo(picoElfSymbolBinding binding, picoElfSymbolType type)
{
    return ((picoElfUchar)binding << 4) + ((picoElfUchar)type & 0xF);
}

// based on https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.symtab.html
picoElfUchar picoElfSymbolTableEntryOther(picoElfSymbolVisibility visibility)
{
    return (picoElfUchar)visibility & 0x3;
}

uint32_t picoElfRelocationEntrySymbol(picoElfClass elfClass, const picoElfRelocationEntry *entry)
{
    if (elfClass == PICO_ELF_CLASS_32) {
        return (uint32_t)(entry->info >> 8);
    } else {
        return (uint32_t)(entry->info >> 32);
    }
}

uint32_t picoElfRelocationEntryType(picoElfClass elfClass, const picoElfRelocationEntry *entry)
{
    if (elfClass == PICO_ELF_CLASS_32) {
        return (uint32_t)(entry->info & 0xff);
    } else {
        return (uint32_t)(entry->info & 0xffffffff);
    }
}


uint64_t picoElfRelocationEntryInfo(uint32_t symbolIndex, uint32_t type, picoElfClass elfClass)
{
    if (elfClass == PICO_ELF_CLASS_32) {
        return ((uint64_t)symbolIndex << 8) + type;
    } else {
        return ((uint64_t)symbolIndex << 32) + (type & 0xffffffff);
    }
}

const char* picoElfStringTableEntry(
    const picoElfUchar* data,
    size_t size,
    const picoElfSectionHeader* stringTableSectionHeader,
    size_t index)
{
    PICO_ASSERT(data);
    PICO_ASSERT(stringTableSectionHeader);
    PICO_ASSERT(stringTableSectionHeader->type == PICO_ELF_SHT_STRTAB);

    const size_t stringOffset = stringTableSectionHeader->fileOffset + index;
    if (stringOffset >= size) {
        return NULL;
    }

    return (const char*)(data + stringOffset);
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
        case PICO_ELF_RESULT_OUT_OF_BOUNDS:
            return "Out of bounds";
        case PICO_ELF_RESULT_SYMBOL_TABLE_SECTION_VARIABLE_SIZE:
            return "Symbol table section has variable size entries";
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
            return "ET_NONE (No file type)";
        case PICO_ELF_ET_REL:
            return "ET_REL (Relocatable file)";
        case PICO_ELF_ET_EXEC:
            return "ET_EXEC (Executable file)";
        case PICO_ELF_ET_DYN:
            return "ET_DYN (Shared object file)";
        case PICO_ELF_ET_CORE:
            return "ET_CORE (Core file)";
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
            return "EM_NONE (No machine)";
        case PICO_ELF_EM_M32:
            return "EM_M32 (AT&T WE 32100)";
        case PICO_ELF_EM_SPARC:
            return "EM_SPARC (SPARC)";
        case PICO_ELF_EM_386:
            return "EM_386 (Intel 80386)";
        case PICO_ELF_EM_68K:
            return "EM_68K (Motorola 68000)";
        case PICO_ELF_EM_88K:
            return "EM_88K (Motorola 88000)";
        case PICO_ELF_EM_860:
            return "EM_860 (Intel 80860)";
        case PICO_ELF_EM_MIPS:
            return "EM_MIPS (MIPS I Architecture)";
        case PICO_ELF_EM_S370:
            return "EM_S370 (IBM System/370 Processor)";
        case PICO_ELF_EM_MIPS_RS3_LE:
            return "EM_MIPS_RS3_LE (MIPS RS3000 Little-endian)";
        case PICO_ELF_EM_PARISC:
            return "EM_PARISC (Hewlett-Packard PA-RISC)";
        case PICO_ELF_EM_VPP500:
            return "EM_VPP500 (Fujitsu VPP500)";
        case PICO_ELF_EM_SPARC32PLUS:
            return "EM_SPARC32PLUS (Enhanced instruction set SPARC)";
        case PICO_ELF_EM_960:
            return "EM_960 (Intel 80960)";
        case PICO_ELF_EM_PPC:
            return "EM_PPC (PowerPC)";
        case PICO_ELF_EM_PPC64:
            return "EM_PPC64 (64-bit PowerPC)";
        case PICO_ELF_EM_S390:
            return "EM_S390 (IBM System/390 Processor)";
        case PICO_ELF_EM_V800:
            return "EM_V800 (NEC V800)";
        case PICO_ELF_EM_FR20:
            return "EM_FR20 (Fujitsu FR20)";
        case PICO_ELF_EM_RH32:
            return "EM_RH32 (TRW RH-32)";
        case PICO_ELF_EM_RCE:
            return "EM_RCE (Motorola RCE)";
        case PICO_ELF_EM_ARM:
            return "EM_ARM (Advanced RISC Machines ARM)";
        case PICO_ELF_EM_ALPHA:
            return "EM_ALPHA (Digital Alpha)";
        case PICO_ELF_EM_SH:
            return "EM_SH (Hitachi SH)";
        case PICO_ELF_EM_SPARCV9:
            return "EM_SPARCV9 (SPARC Version 9)";
        case PICO_ELF_EM_TRICORE:
            return "EM_TRICORE (Siemens TriCore embedded processor)";
        case PICO_ELF_EM_ARC:
            return "EM_ARC (Argonaut RISC Core)";
        case PICO_ELF_EM_H8_300:
            return "EM_H8_300 (Hitachi H8/300)";
        case PICO_ELF_EM_H8_300H:
            return "EM_H8_300H (Hitachi H8/300H)";
        case PICO_ELF_EM_H8S:
            return "EM_H8S (Hitachi H8S)";
        case PICO_ELF_EM_H8_500:
            return "EM_H8_500 (Hitachi H8/500)";
        case PICO_ELF_EM_IA_64:
            return "EM_IA_64 (Intel IA-64 processor architecture)";
        case PICO_ELF_EM_MIPS_X:
            return "EM_MIPS_X (Stanford MIPS-X)";
        case PICO_ELF_EM_COLDFIRE:
            return "EM_COLDFIRE (Motorola ColdFire)";
        case PICO_ELF_EM_68HC12:
            return "EM_68HC12 (Motorola M68HC12)";
        case PICO_ELF_EM_MMA:
            return "EM_MMA (Fujitsu MMA Multimedia Accelerator)";
        case PICO_ELF_EM_PCP:
            return "EM_PCP (Siemens PCP)";
        case PICO_ELF_EM_NCPU:
            return "EM_NCPU (Sony nCPU embedded RISC processor)";
        case PICO_ELF_EM_NDR1:
            return "EM_NDR1 (Denso NDR1 microprocessor)";
        case PICO_ELF_EM_STARCORE:
            return "EM_STARCORE (Motorola Star*Core processor)";
        case PICO_ELF_EM_ME16:
            return "EM_ME16 (Toyota ME16 processor)";
        case PICO_ELF_EM_ST100:
            return "EM_ST100 (STMicroelectronics ST100 processor)";
        case PICO_ELF_EM_TINYJ:
            return "EM_TINYJ (Advanced Logic Corp. TinyJ embedded processor family)";
        case PICO_ELF_EM_X86_64:
            return "EM_X86_64 (AMD x86-64 architecture)";
        case PICO_ELF_EM_PDSP:
            return "EM_PDSP (Sony DSP Processor)";
        case PICO_ELF_EM_PDP10:
            return "EM_PDP10 (Digital Equipment Corp. PDP-10)";
        case PICO_ELF_EM_PDP11:
            return "EM_PDP11 (Digital Equipment Corp. PDP-11)";
        case PICO_ELF_EM_FX66:
            return "EM_FX66 (Siemens FX66 microcontroller)";
        case PICO_ELF_EM_ST9PLUS:
            return "EM_ST9PLUS (STMicroelectronics ST9+ 8/16 bit microcontroller)";
        case PICO_ELF_EM_ST7:
            return "EM_ST7 (STMicroelectronics ST7 8-bit microcontroller)";
        case PICO_ELF_EM_68HC16:
            return "EM_68HC16 (Motorola MC68HC16 Microcontroller)";
        case PICO_ELF_EM_68HC11:
            return "EM_68HC11 (Motorola MC68HC11 Microcontroller)";
        case PICO_ELF_EM_68HC08:
            return "EM_68HC08 (Motorola MC68HC08 Microcontroller)";
        case PICO_ELF_EM_68HC05:
            return "EM_68HC05 (Motorola MC68HC05 Microcontroller)";
        case PICO_ELF_EM_SVX:
            return "EM_SVX (Silicon Graphics SVx)";
        case PICO_ELF_EM_ST19:
            return "EM_ST19 (STMicroelectronics ST19 8-bit microcontroller)";
        case PICO_ELF_EM_VAX:
            return "EM_VAX (Digital VAX)";
        case PICO_ELF_EM_CRIS:
            return "EM_CRIS (Axis Communications 32-bit embedded processor)";
        case PICO_ELF_EM_JAVELIN:
            return "EM_JAVELIN (Infineon Technologies 32-bit embedded processor)";
        case PICO_ELF_EM_FIREPATH:
            return "EM_FIREPATH (Element 14 64-bit DSP Processor)";
        case PICO_ELF_EM_ZSP:
            return "EM_ZSP (LSI Logic 16-bit DSP Processor)";
        case PICO_ELF_EM_MMIX:
            return "EM_MMIX (Donald Knuth's educational 64-bit processor)";
        case PICO_ELF_EM_HUANY:
            return "EM_HUANY (Harvard University machine-independent object files)";
        case PICO_ELF_EM_PRISM:
            return "EM_PRISM (SiTera Prism)";
        case PICO_ELF_EM_AVR:
            return "EM_AVR (Atmel AVR 8-bit microcontroller)";
        case PICO_ELF_EM_FR30:
            return "EM_FR30 (Fujitsu FR30)";
        case PICO_ELF_EM_D10V:
            return "EM_D10V (Mitsubishi D10V)";
        case PICO_ELF_EM_D30V:
            return "EM_D30V (Mitsubishi D30V)";
        case PICO_ELF_EM_V850:
            return "EM_V850 (NEC v850)";
        case PICO_ELF_EM_M32R:
            return "EM_M32R (Mitsubishi M32R)";
        case PICO_ELF_EM_MN10300:
            return "EM_MN10300 (Matsushita MN10300)";
        case PICO_ELF_EM_MN10200:
            return "EM_MN10200 (Matsushita MN10200)";
        case PICO_ELF_EM_PJ:
            return "EM_PJ (picoJava)";
        case PICO_ELF_EM_OPENRISC:
            return "EM_OPENRISC (OpenRISC 32-bit embedded processor)";
        case PICO_ELF_EM_ARC_A5:
            return "EM_ARC_A5 (ARC Cores Tangent-A5)";
        case PICO_ELF_EM_XTENSA:
            return "EM_XTENSA (Tensilica Xtensa Architecture)";
        case PICO_ELF_EM_VIDEOCORE:
            return "EM_VIDEOCORE (Alphamosaic VideoCore processor)";
        case PICO_ELF_EM_TMM_GPP:
            return "EM_TMM_GPP (Thompson Multimedia General Purpose Processor)";
        case PICO_ELF_EM_NS32K:
            return "EM_NS32K (National Semiconductor 32000 series)";
        case PICO_ELF_EM_TPC:
            return "EM_TPC (Tenor Network TPC processor)";
        case PICO_ELF_EM_SNP1K:
            return "EM_SNP1K (Trebia SNP 1000 processor)";
        case PICO_ELF_EM_ST200:
            return "EM_ST200 (STMicroelectronics ST200 microcontroller)";
        case PICO_ELF_EM_RISCV:
            return "EM_RISCV (RISC-V)";
        default:
            return "Unknown machine";
    }
}

const char *picoElfSectionTypeToString(picoElfSectionType sectionType)
{
    switch (sectionType) {
        case PICO_ELF_SHT_NULL:
            return "SHT_NULL (unused)";
        case PICO_ELF_SHT_PROGBITS:
            return "SHT_PROGBITS (program data)";
        case PICO_ELF_SHT_SYMTAB:
            return "SHT_SYMTAB (symbol table)";
        case PICO_ELF_SHT_STRTAB:
            return "SHT_STRTAB (string table)";
        case PICO_ELF_SHT_RELA:
            return "SHT_RELA (relocation entries with addends)";
        case PICO_ELF_SHT_HASH:
            return "SHT_HASH (hash table)";
        case PICO_ELF_SHT_DYNAMIC:
            return "SHT_DYNAMIC (dynamic linking information)";
        case PICO_ELF_SHT_NOTE:
            return "SHT_NOTE (note sections)";
        case PICO_ELF_SHT_NOBITS:
            return "SHT_NOBITS (no space section)";
        case PICO_ELF_SHT_REL:
            return "SHT_REL (relocation entries without addends)";
        case PICO_ELF_SHT_SHLIB:
            return "SHT_SHLIB (shared library section)";
        case PICO_ELF_SHT_DYNSYM:
            return "SHT_DYNSYM (dynamic symbol table)";
        case PICO_ELF_SHT_INIT_ARRAY:
            return "SHT_INIT_ARRAY (initialization function pointers)";
        case PICO_ELF_SHT_FINI_ARRAY:
            return "SHT_FINI_ARRAY (finalization function pointers)";
        case PICO_ELF_SHT_PREINIT_ARRAY:
            return "SHT_PREINIT_ARRAY (pre-initialization function pointers)";
        case PICO_ELF_SHT_GROUP:
            return "SHT_GROUP (section group)";
        case PICO_ELF_SHT_SYMTAB_SHNDX:
            return "SHT_SYMTAB_SHNDX (extended section indices)";
        default:
            if (sectionType >= PICO_ELF_SHT_LOOS && sectionType <= PICO_ELF_SHT_HIOS)
                return "OS-specific";
            if (sectionType >= PICO_ELF_SHT_LOPROC && sectionType <= PICO_ELF_SHT_HIPROC)
                return "Processor-specific";
            return "Unknown";
    }
}

const char *picoElfSymbolBindingToString(picoElfSymbolBinding binding)
{
    switch (binding) {
        case PICO_ELF_STB_LOCAL:
            return "STB_LOCAL (Local symbol)";
        case PICO_ELF_STB_GLOBAL:
            return "STB_GLOBAL (Global symbol)";
        case PICO_ELF_STB_WEAK:
            return "STB_WEAK (Weak symbol)";
        default:
            if (binding >= PICO_ELF_STB_LOOS && binding <= PICO_ELF_STB_HIOS)
                return "OS-specific";
            if (binding >= PICO_ELF_STB_LOPROC && binding <= PICO_ELF_STB_HIPROC)
                return "Processor-specific";
            return "Unknown binding";
    }
}

const char *picoElfSymbolTypeToString(picoElfSymbolType type)
{
    switch (type) {
        case PICO_ELF_STT_NOTYPE:
            return "STT_NOTYPE (No type)";
        case PICO_ELF_STT_OBJECT:
            return "STT_OBJECT (Data object)";
        case PICO_ELF_STT_FUNC:
            return "STT_FUNC (Function)";
        case PICO_ELF_STT_SECTION:
            return "STT_SECTION (Section)";
        case PICO_ELF_STT_FILE:
            return "STT_FILE (File)";
        default:
            if (type >= PICO_ELF_STT_LOOS && type <= PICO_ELF_STT_HIOS)
                return "OS-specific";
            if (type >= PICO_ELF_STT_LOPROC && type <= PICO_ELF_STT_HIPROC)
                return "Processor-specific";
            return "Unknown type";
    }
}

const char *picoElfSymbolVisibilityToString(picoElfSymbolVisibility visibility)
{
    switch (visibility) {
        case PICO_ELF_STV_DEFAULT:
            return "STV_DEFAULT (Default visibility)";
        case PICO_ELF_STV_INTERNAL:
            return "STV_INTERNAL (Internal visibility)";
        case PICO_ELF_STV_HIDDEN:
            return "STV_HIDDEN (Hidden visibility)";
        case PICO_ELF_STV_PROTECTED:
            return "STV_PROTECTED (Protected visibility)";
        default:
            return "Unknown visibility";
    }
}

const char *picoElfProgramTypeToString(picoElfProgramType type)
{
    switch (type) {
        case PICO_ELF_PT_NULL:
            return "PT_NULL (unused)";
        case PICO_ELF_PT_LOAD:
            return "PT_LOAD (loadable segment)";
        case PICO_ELF_PT_DYNAMIC:
            return "PT_DYNAMIC (dynamic linking information)";
        case PICO_ELF_PT_INTERP:
            return "PT_INTERP (interpreter information)";
        case PICO_ELF_PT_NOTE:
            return "PT_NOTE (note segment)";
        case PICO_ELF_PT_SHLIB:
            return "PT_SHLIB (reserved)";
        case PICO_ELF_PT_PHDR:
            return "PT_PHDR (program header table)";
        default:
            if (type >= PICO_ELF_PT_LOOS && type <= PICO_ELF_PT_HIOS)
                return "OS-specific";
            if (type >= PICO_ELF_PT_LOPROC && type <= PICO_ELF_PT_HIPROC)
                return "Processor-specific";
            return "Unknown";
    }
}

const char *picoElfProgramFlagsToString(uint64_t flags)
{
    static char buffer[1024];
    buffer[0] = '\0';

#define PICO_ELF_APPEND_FLAG(flag, desc)     \
    do {                               \
        if (flags & flag) {            \
            if (buffer[0] != '\0') {   \
                strcat(buffer, " | "); \
            }                          \
            strcat(buffer, desc);     \
        }                              \
    } while (0)

    PICO_ELF_APPEND_FLAG(PICO_ELF_PF_X, "PF_X");
    PICO_ELF_APPEND_FLAG(PICO_ELF_PF_W, "PF_W");
    PICO_ELF_APPEND_FLAG(PICO_ELF_PF_R, "PF_R");

#undef PICO_ELF_APPEND_FLAG

    return buffer;
}

const char *picoElfSectionFlagsToString(picoElfSectionFlags flags)
{
    static char buffer[1024];
    buffer[0] = '\0';

#define PICO_ELF_APPEND_FLAG(flag, desc)     \
    do {                               \
        if (flags & flag) {            \
            if (buffer[0] != '\0') {   \
                strcat(buffer, " | "); \
            }                          \
            strcat(buffer, desc);     \
        }                              \
    } while (0)

    PICO_ELF_APPEND_FLAG(PICO_ELF_SHF_WRITE, "SHF_WRITE");
    PICO_ELF_APPEND_FLAG(PICO_ELF_SHF_ALLOC, "SHF_ALLOC");
    PICO_ELF_APPEND_FLAG(PICO_ELF_SHF_EXECINSTR, "SHF_EXECINSTR");
    PICO_ELF_APPEND_FLAG(PICO_ELF_SHF_MERGE, "SHF_MERGE");
    PICO_ELF_APPEND_FLAG(PICO_ELF_SHF_STRINGS, "SHF_STRINGS");
    PICO_ELF_APPEND_FLAG(PICO_ELF_SHF_INFO_LINK, "SHF_INFO_LINK");
    PICO_ELF_APPEND_FLAG(PICO_ELF_SHF_LINK_ORDER, "SHF_LINK_ORDER");
    PICO_ELF_APPEND_FLAG(PICO_ELF_SHF_OS_NONCONFORMING, "SHF_OS_NONCONFORMING");
    PICO_ELF_APPEND_FLAG(PICO_ELF_SHF_GROUP, "SHF_GROUP");
    PICO_ELF_APPEND_FLAG(PICO_ELF_SHF_TLS, "SHF_TLS");
    PICO_ELF_APPEND_FLAG(PICO_ELF_SHF_COMPRESSED, "SHF_COMPRESSED");

#undef PICO_ELF_APPEND_FLAG

    return buffer;
}

void picoElfIdentifierDebugPrint(int padding, const picoElfIdentifier *ident)
{
    PICO_ASSERT(ident);

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
    PICO_ASSERT(header);

    PICO_ELF_LOG("%*sELF Identifier:\n", padding, "");
    picoElfIdentifierDebugPrint(padding + 2, &header->ident);
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

void picoElfSectionHeaderDebugPrint(int padding, const picoElfSectionHeader *sectionHeader)
{
    PICO_ASSERT(sectionHeader);

    PICO_ELF_LOG("%*sName Offset: %zu\n", padding, "", sectionHeader->nameOffset);
    PICO_ELF_LOG("%*sType: %s\n", padding, "", picoElfSectionTypeToString(sectionHeader->type));
    PICO_ELF_LOG("%*sFlags: %s (0x%" PRIx64 ")\n", padding, "", picoElfSectionFlagsToString((picoElfSectionFlags)sectionHeader->flags), sectionHeader->flags);
    PICO_ELF_LOG("%*sVirtual Address: 0x%llX\n", padding, "", (unsigned long long)sectionHeader->virtualAddress);
    PICO_ELF_LOG("%*sFile Offset: 0x%llX\n", padding, "", (unsigned long long)sectionHeader->fileOffset);
    PICO_ELF_LOG("%*sSection Size: %zu bytes\n", padding, "", sectionHeader->sectionSize);
    PICO_ELF_LOG("%*sLink: %u\n", padding, "", sectionHeader->link);
    PICO_ELF_LOG("%*sInfo: %u\n", padding, "", sectionHeader->info);
    PICO_ELF_LOG("%*sAddress Alignment: 0x%llX\n", padding, "", (unsigned long long)sectionHeader->addressAlignment);
    PICO_ELF_LOG("%*sEntry Size: %zu bytes\n", padding, "", sectionHeader->entrySize);
}

void picoElfSymbolTableEntryDebugPrint(int padding, const picoElfSymbolTableEntry *symbolTableEntry)
{
    PICO_ASSERT(symbolTableEntry);

    PICO_ELF_LOG("%*sName Offset: %zu\n", padding, "", symbolTableEntry->nameOffset);
    PICO_ELF_LOG("%*sValue: 0x%" PRIx64 "\n", padding, "", symbolTableEntry->value);
    PICO_ELF_LOG("%*sSize: %zu bytes\n", padding, "", symbolTableEntry->size);
    PICO_ELF_LOG(
        "%*sInfo: (Binding: %s, Type: %s)\n",
        padding,
        "",
        picoElfSymbolBindingToString(picoElfSymbolTableEntryBinding(symbolTableEntry)), 
        picoElfSymbolTypeToString(picoElfSymbolTableEntryType(symbolTableEntry)));
    PICO_ELF_LOG("%*sVisibility: %s\n", padding, "", picoElfSymbolVisibilityToString(picoElfSymbolTableEntryVisibility(symbolTableEntry)));
    PICO_ELF_LOG("%*sSection Index: %u\n", padding, "", symbolTableEntry->sectionIndex);
}

void picoElfRelocationEntryDebugPrint(int padding, const picoElfRelocationEntry *relocationEntry)
{
    PICO_ASSERT(relocationEntry);

    PICO_ELF_LOG("%*sOffset: 0x%" PRIx64 "\n", padding, "", relocationEntry->offset);
    PICO_ELF_LOG("%*sInfo: 0x%" PRIx64 "\n", padding, "", relocationEntry->info);
    if (relocationEntry->isRela) {
        PICO_ELF_LOG("%*sAddend: %" PRId64 "\n", padding, "", relocationEntry->addend);
    }
}

void picoElfProgramHeaderDebugPrint(int padding, const picoElfProgramHeader *programHeader)
{
    PICO_ASSERT(programHeader);

    PICO_ELF_LOG("%*sType: %s\n", padding, "", picoElfProgramTypeToString(programHeader->type));
    PICO_ELF_LOG("%*sFlags: %s (0x%" PRIx64 ")\n", padding, "", picoElfProgramFlagsToString(programHeader->flags), programHeader->flags);
    PICO_ELF_LOG("%*sOffset: 0x%" PRIx64 "\n", padding, "", programHeader->offset);
    PICO_ELF_LOG("%*sVirtual Address: 0x%" PRIx64 "\n", padding, "", programHeader->virtualAddress);
    PICO_ELF_LOG("%*sPhysical Address: 0x%" PRIx64 "\n", padding, "", programHeader->physicalAddress);
    PICO_ELF_LOG("%*sFile Size: %zu bytes\n", padding, "", programHeader->fileSize);
    PICO_ELF_LOG("%*sMemory Size: %zu bytes\n", padding, "", programHeader->memorySize);
    PICO_ELF_LOG("%*sAlignment: 0x%" PRIx64 "\n", padding, "", programHeader->alignment);
}

// undefine internal parsing macros to avoid polluting the global namespace
#undef PICO_ELF__PARSE_U64
#undef PICO_ELF__PARSE_S64
#undef PICO_ELF__PARSE_U32
#undef PICO_ELF__PARSE_S32
#undef PICO_ELF__PARSE_U16
#undef PICO_ELF__PARSE_U8
#undef PICO_ELF__WRITE_U64
#undef PICO_ELF__WRITE_S64
#undef PICO_ELF__WRITE_U32
#undef PICO_ELF__WRITE_S32
#undef PICO_ELF__WRITE_U16
#undef PICO_ELF__WRITE_S16
#undef PICO_ELF__WRITE_U8
#undef PICO_ELF__WRITE_S8
#undef PICO_ELF__PARSE_U8

#endif // PICO_ELF_IMPLEMENTATION
#endif // PICO_ELF_H
