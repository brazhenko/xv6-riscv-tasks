typedef uint32 fdt32_t;
typedef uint64 fdt64_t;

struct fdt_header {
    fdt32_t magic;
    fdt32_t totalsize;

    fdt32_t off_dt_struct;
    fdt32_t off_dt_strings;
    
    fdt32_t off_mem_rsvmap;
    
    fdt32_t version;
    fdt32_t last_comp_version;
    fdt32_t boot_cpuid_phys;
    fdt32_t size_dt_strings;
    fdt32_t size_dt_struct;
};

struct fdt_reserve_entry {
	fdt64_t address;
	fdt64_t size;
};

struct fdt_phmemory_entry {
    fdt64_t address;
	fdt64_t size;
};

struct fdt_node_header {
	fdt32_t tag;
	char name[];
};

struct fdt_property {
	fdt32_t tag;
	fdt32_t len;
	fdt32_t nameoff;
	char data[];
};


#define FDT_MAGIC	0xd00dfeed	/* 4: version, 4: total size */
#define FDT_TAGSIZE	sizeof(fdt32_t)

#define FDT_BEGIN_NODE	0x1		/* Start node: full name */
#define FDT_END_NODE	0x2		/* End node */
#define FDT_PROP	0x3		/* Property: name off,
					   size, content */
#define FDT_NOP		0x4		/* nop */
#define FDT_END		0x9

#define FDT_V1_SIZE	(7*sizeof(fdt32_t))
#define FDT_V2_SIZE	(FDT_V1_SIZE + sizeof(fdt32_t))
#define FDT_V3_SIZE	(FDT_V2_SIZE + sizeof(fdt32_t))
#define FDT_V16_SIZE	FDT_V3_SIZE
#define FDT_V17_SIZE	(FDT_V16_SIZE + sizeof(fdt32_t))


uint32 swap_endian(uint32 value) {
    return ((value << 24) & 0xFF000000) |
           ((value << 8) & 0x00FF0000) |
           ((value >> 8) & 0x0000FF00) |
           ((value >> 24) & 0x000000FF);
}

uint64 swap_endian_64(uint64 value) {
    return ((value << 56)                & 0xFF00000000000000ULL) |
           ((value << 40) & 0x00FF000000000000ULL) |
           ((value << 24) & 0x0000FF0000000000ULL) |
           ((value << 8)  & 0x000000FF00000000ULL) |
           ((value >> 8)  & 0x00000000FF000000ULL) |
           ((value >> 24) & 0x0000000000FF0000ULL) |
           ((value >> 40) & 0x000000000000FF00ULL) |
           ((value >> 56)                & 0x00000000000000FFULL);
}

uint64 round_up_to_4(uint64 value) {
    return (value + 3) & ~3U;
}

static inline void parse_dtb()
{
    void *start_ptr = (void*)a7reg();

    struct fdt_header* head = (struct fdt_header *)(start_ptr);

    void *end_ptr = start_ptr + swap_endian(head->totalsize);
    
    // 0xd00dfeed

    printf("magic: %a\n", swap_endian(head->magic));
    printf("start: %a\n", head);
    printf("totalsize: %u\n", swap_endian(head->totalsize));
    printf("version: %u\n", swap_endian(head->version));
    printf("cpu id: %u\n", swap_endian(head->boot_cpuid_phys));
    printf("strings size: %u\n", swap_endian(head->size_dt_strings));
    printf("struct size: %u\n", swap_endian(head->size_dt_struct));
    printf("struct size: %u\n", swap_endian(head->off_mem_rsvmap));

    void *mem_reserved_start = start_ptr + swap_endian(head->off_mem_rsvmap);
    void *dt_struct_start = start_ptr + swap_endian(head->off_dt_struct);
    void *dt_string_start = start_ptr + swap_endian(head->off_dt_strings);

    (void)end_ptr;

    printf("== reserved memory ==\n");
    while (1) {
        struct fdt_reserve_entry* entry = (struct fdt_reserve_entry *)(mem_reserved_start);

        printf("ptr: %p, begin: %p, size: %p\n", entry, swap_endian_64(entry->address), swap_endian_64(entry->size));
        mem_reserved_start += sizeof(*entry);
        
        if (entry->address == 0 && entry->address == 0) {
            break;
        }
    }


    printf("== struct ==\n");

    int memory = 0;

    while (1) {
        struct fdt_property *ptr = (struct fdt_property *) dt_struct_start;

        if (swap_endian(ptr->tag) == FDT_END) {
            printf("end\n");
            break;
        }

        switch (swap_endian( ptr->tag))
        {
        case FDT_BEGIN_NODE: {
            struct fdt_node_header *begin_node = (struct fdt_node_header *)dt_struct_start;

            uint64 add = (strlen(&begin_node->name[0]));

            if (add % 4 == 0) {
                add += 4;
            } else {
                add = round_up_to_4(add);
            }
            if (strncmp("memory", &begin_node->name[0], 6) == 0) {
                memory = 1;
                printf("1. tok: %u,  name: [%s], ptr: %p, strlen: %d, add: %d\n",
                    swap_endian(begin_node->tag),
                    &begin_node->name[0],
                    begin_node,
                    strlen(&begin_node->name[0]),
                add); 
            }
            


            dt_struct_start += sizeof(fdt32_t) + add;
            break;

        }
        case FDT_END_NODE:
            memory = 0;
            dt_struct_start += sizeof(fdt32_t);
            break;
        case FDT_PROP: {

            if (memory && strncmp("reg", (const char *)(dt_string_start + swap_endian(ptr->nameoff)), 3) == 0) {

                struct fdt_reserve_entry *entry = (struct fdt_reserve_entry *)(dt_struct_start + 4 * 3);
                uint32 last = swap_endian(ptr->len);

                while (last) {
                    printf("2. begin: %p, size: %p, entry: %p\n", 
                        swap_endian_64(entry->address),
                        swap_endian_64(entry->size),
                        entry);     
                    ++entry;
                    last -= sizeof(struct fdt_reserve_entry);
                }

            }

            dt_struct_start += sizeof(fdt32_t) * 3 + round_up_to_4(swap_endian(ptr->len));

            break;

        }
        case FDT_NOP:
            printf("nop\n");
            dt_struct_start += sizeof(fdt32_t);
            break;
        default:
            printf("panic: %u\n", swap_endian( ptr->tag));
            // unreachable
            break;
        }
    }

}