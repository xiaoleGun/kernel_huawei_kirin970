#ifndef _ASM_POWERPC_SETUP_H
#define _ASM_POWERPC_SETUP_H

#include <uapi/asm/setup.h>

#ifndef __ASSEMBLY__
extern void ppc_printk_progress(char *s, unsigned short hex);

extern unsigned int rtas_data;
extern unsigned long long memory_limit;
extern bool init_mem_is_free;
extern unsigned long klimit;
extern void *zalloc_maybe_bootmem(size_t size, gfp_t mask);

struct device_node;
extern void note_scsi_host(struct device_node *, void *);

/* Used in very early kernel initialization. */
extern unsigned long reloc_offset(void);
extern unsigned long add_reloc_offset(unsigned long);
extern void reloc_got2(unsigned long);

#define PTRRELOC(x)	((typeof(x)) add_reloc_offset((unsigned long)(x)))

void check_for_initrd(void);
void initmem_init(void);
void setup_panic(void);
#define ARCH_PANIC_TIMEOUT 180

#ifdef CONFIG_PPC_PSERIES
extern void pseries_enable_reloc_on_exc(void);
extern void pseries_disable_reloc_on_exc(void);
extern void pseries_big_endian_exceptions(void);
extern void pseries_little_endian_exceptions(void);
#else
static inline void pseries_enable_reloc_on_exc(void) {}
static inline void pseries_disable_reloc_on_exc(void) {}
static inline void pseries_big_endian_exceptions(void) {}
static inline void pseries_little_endian_exceptions(void) {}
#endif /* CONFIG_PPC_PSERIES */

#endif /* !__ASSEMBLY__ */

#endif	/* _ASM_POWERPC_SETUP_H */

