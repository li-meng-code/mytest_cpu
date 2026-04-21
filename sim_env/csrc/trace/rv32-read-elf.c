/* Now this file is only for riscv32 isa */
#include <common.h>

#ifdef CONFIG_FTRACE
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>

/* Structure to store basic ELF file information */
typedef struct {
    int fd;
    Elf32_Ehdr ehdr;
    Elf32_Shdr *shdrs;
    char *shstrtab;
    Elf32_Off symtab_offset, strtab_offset;
    Elf32_Word symtab_size, strtab_size;
} ElfFile;

/* Structiore to store function symbols */
#define FUNC_HASH_SIZE 4096 
#define FUNC_MAX_NAME 64
typedef struct {
    char name[FUNC_MAX_NAME];
    uint32_t addr;
    bool used;
} FuncEntry;

static FuncEntry func_hash_table [FUNC_HASH_SIZE] = {0};

/*---------------------------------------------------------*/
/*                   Static helper functions               */
/*---------------------------------------------------------*/
/* maintaining hash table */
static inline uint32_t hash_func(uint32_t addr) {
    return (addr * 2654435761u) % FUNC_HASH_SIZE;
}

static void func_hash_insert(const char* name, uint32_t addr) {
    uint32_t h = hash_func(addr);
    uint32_t start = h;
    while (func_hash_table[h].used) {
        h = (h + 1) % FUNC_HASH_SIZE;
        if (h == start) assert(0);
    }
    strncpy(func_hash_table[h].name, name, FUNC_MAX_NAME - 1);
    func_hash_table[h].name[FUNC_MAX_NAME - 1] = '\0';
    func_hash_table[h].used = true;
    func_hash_table[h].addr = addr;
}

const char *func_hash_lookup(uint32_t addr) {
    uint32_t h = hash_func(addr);
    uint32_t start = h;
    while (func_hash_table[h].used) {
        if (func_hash_table[h].addr == addr) 
            return func_hash_table[h].name;
        h = (h + 1) % FUNC_HASH_SIZE;
        if (h == start) break;// loop
    }
    return NULL;
}

/* Read and validate the ELF header */
static void read_elf_header(ElfFile *elf) {
    if (pread(elf->fd, &elf->ehdr, sizeof(Elf32_Ehdr), 0) != sizeof(Elf32_Ehdr)) {
        perror("pread Elf32_Ehdr");
        exit(1);
    }

    const unsigned char expected_magic[EI_NIDENT] = {
        0x7f, 'E', 'L', 'F', 0x01, 0x01, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    for (int i = 0; i < EI_NIDENT; i++) {
        Assert(elf->ehdr.e_ident[i] == expected_magic[i], "riscv32-elf check failed");
    }

    Log("ELF header magic verified.");
    Log("Section header offset: 0x%08x", (unsigned int)elf->ehdr.e_shoff);
    Log("Section header string table index: %u", elf->ehdr.e_shstrndx);
}

/* Load all section headers and the section header string table (.shstrtab) */
static void read_section_headers(ElfFile *elf) {
    size_t size = elf->ehdr.e_shnum * sizeof(Elf32_Shdr);
    elf->shdrs = (Elf32_Shdr*)malloc(size);
    assert(elf->shdrs);

    if (pread(elf->fd, elf->shdrs, size, elf->ehdr.e_shoff) != (ssize_t)size) {
        perror("pread Elf32_Shdr");
        exit(1);
    }

    Elf32_Shdr *shstr = &elf->shdrs[elf->ehdr.e_shstrndx];
    elf->shstrtab = (char *)malloc(shstr->sh_size);
    assert(elf->shstrtab);

    if (pread(elf->fd, elf->shstrtab, shstr->sh_size, shstr->sh_offset) != (ssize_t)shstr->sh_size) {
        perror("pread shstrtab");
        exit(1);
    }

    Log("Loaded section headers and shstrtab (size=%u).", shstr->sh_size);
}

/* Locate the .symtab and .strtab sections and store their offsets and sizes */
static void find_symtab_and_strtab(ElfFile *elf) {
    for (int i = 0; i < elf->ehdr.e_shnum; i++) {
        Elf32_Shdr *sh = &elf->shdrs[i];
        const char *name = elf->shstrtab + sh->sh_name;

        if (strcmp(name, ".symtab") == 0) {
            elf->symtab_offset = sh->sh_offset;
            elf->symtab_size = sh->sh_size;
            Log(".symtab found, offset=0x%08x, size=%u bytes", elf->symtab_offset, elf->symtab_size);
        } else if (strcmp(name, ".strtab") == 0) {
            elf->strtab_offset = sh->sh_offset;
            elf->strtab_size = sh->sh_size;
            Log(".strtab found, offset=0x%08x, size=%u bytes", elf->strtab_offset, elf->strtab_size);
        }

        if (elf->symtab_size && elf->strtab_size) break;
    }

    if (!elf->symtab_size || !elf->strtab_size) {
        fprintf(stderr, "Error: .symtab or .strtab not found.\n");
        exit(1);
    }
}

/* Read the symbol table and print all function symbols */
static void build_func_hash(ElfFile *elf) {
    Elf32_Sym *symtab = (Elf32_Sym *)malloc(elf->symtab_size);
    char *strtab = (char *)malloc(elf->strtab_size);
    assert(symtab && strtab);

    if (pread(elf->fd, symtab, elf->symtab_size, elf->symtab_offset) != (ssize_t)elf->symtab_size) {
        perror("pread symtab");
        exit(1);
    }
    if (pread(elf->fd, strtab, elf->strtab_size, elf->strtab_offset) != (ssize_t)elf->strtab_size) {
        perror("pread strtab");
        exit(1);
    }

    int nr_symbol = elf->symtab_size / sizeof(Elf32_Sym);
    Log("Scanning %d symbols...", nr_symbol);
    for (int i = 0; i < nr_symbol; i++) {
        Elf32_Sym *sym = &symtab[i];
        const char *name = strtab + sym->st_name;
        unsigned char st_type = sym->st_info & 0x0f;
        if (st_type == STT_FUNC) {
            Log("%s@0x%08x", name, sym->st_value);
            func_hash_insert(name, sym->st_value);
        }
    }

    free(symtab);
    free(strtab);
}

/* Free all allocated memory and close the file descriptor */
static void cleanup_elf(ElfFile *elf) {
    if (elf->shdrs) free(elf->shdrs);
    if (elf->shstrtab) free(elf->shstrtab);
    if (elf->fd >= 0) close(elf->fd);
}

/*---------------------------------------------------------*/
/*                          main                           */
/*---------------------------------------------------------*/

int ftrace_elf_analyze(const char *elf_file) {
    assert(elf_file);
    ElfFile elf = {0};
    elf.fd = open(elf_file, O_RDONLY);
    if (elf.fd < 0) {
        perror("open elf_file");
        Assert(0, "Cannot open '%s'", elf_file);
        return -1;
    }

    read_elf_header(&elf);
    read_section_headers(&elf);
    find_symtab_and_strtab(&elf);
    build_func_hash(&elf);
    cleanup_elf(&elf);
    return 0;
}

#endif /* CONFIG_FTRACE */
