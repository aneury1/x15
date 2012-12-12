/*
 * Copyright (c) 2010, 2011, 2012 Richard Braun.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * TODO Comment.
 */

#ifndef _X86_PMAP_H
#define _X86_PMAP_H

#include <kern/macros.h>

/*
 * Page table entry flags.
 */
#define PMAP_PTE_P      0x00000001
#define PMAP_PTE_RW     0x00000002
#define PMAP_PTE_US     0x00000004
#define PMAP_PTE_PWT    0x00000008
#define PMAP_PTE_PCD    0x00000010
#define PMAP_PTE_A      0x00000020
#define PMAP_PTE_D      0x00000040
#define PMAP_PTE_PS     0x00000080
#define PMAP_PTE_G      0x00000100

/*
 * Page translation hierarchy properties.
 */
#define PMAP_L1_MASK    (PMAP_PA_MASK | PMAP_PTE_D | PMAP_PTE_A \
                         | PMAP_PTE_PCD | PMAP_PTE_PWT | PMAP_PTE_US \
                         | PMAP_PTE_RW | PMAP_PTE_P)
#define PMAP_L2_MASK    (PMAP_PA_MASK | PMAP_PTE_A | PMAP_PTE_PCD \
                         | PMAP_PTE_PWT | PMAP_PTE_US | PMAP_PTE_RW \
                         | PMAP_PTE_P)

#ifdef __LP64__
#define PMAP_NR_RPTPS   1
#define PMAP_NR_LEVELS  4
#define PMAP_L1_BITS    9
#define PMAP_L2_BITS    9
#define PMAP_L3_BITS    9
#define PMAP_L4_BITS    9
#define PMAP_VA_MASK    DECL_CONST(0x0000ffffffffffff, UL)
#define PMAP_PA_MASK    DECL_CONST(0x000ffffffffff000, UL)
#define PMAP_L3_MASK    PMAP_L2_MASK
#define PMAP_L4_MASK    PMAP_L2_MASK
#else /* __LP64__ */
#ifdef X86_PAE
#define PMAP_NR_RPTPS   4   /* Assume two levels with a 4-page root table */
#define PMAP_NR_LEVELS  2
#define PMAP_L1_BITS    9
#define PMAP_L2_BITS    11
#define PMAP_VA_MASK    DECL_CONST(0xffffffff, UL)
#define PMAP_PA_MASK    DECL_CONST(0x000ffffffffff000, ULL)
#else /* X86_PAE */
#define PMAP_NR_RPTPS   1
#define PMAP_NR_LEVELS  2
#define PMAP_L1_BITS    10
#define PMAP_L2_BITS    10
#define PMAP_VA_MASK    DECL_CONST(0xffffffff, UL)
#define PMAP_PA_MASK    DECL_CONST(0xfffff000, UL)
#endif /* X86_PAE */
#endif /* __LP64__ */

#define PMAP_L1_SHIFT   12
#define PMAP_L2_SHIFT   (PMAP_L1_SHIFT + PMAP_L1_BITS)
#define PMAP_L3_SHIFT   (PMAP_L2_SHIFT + PMAP_L2_BITS)
#define PMAP_L4_SHIFT   (PMAP_L3_SHIFT + PMAP_L3_BITS)

#define PMAP_L1_NR_PTES (1 << PMAP_L1_BITS)
#define PMAP_L2_NR_PTES (1 << PMAP_L2_BITS)
#define PMAP_L3_NR_PTES (1 << PMAP_L3_BITS)
#define PMAP_L4_NR_PTES (1 << PMAP_L4_BITS)

/*
 * Size of the recursive mapping of PTEs.
 */
#ifdef __LP64__
#define PMAP_PTEMAP_SIZE DECL_CONST(0x8000000000, UL)
#else /* __LP64__ */
#ifdef X86_PAE
#define PMAP_PTEMAP_SIZE DECL_CONST(0x800000, UL)
#else /* X86_PAE */
#define PMAP_PTEMAP_SIZE DECL_CONST(0x400000, UL)
#endif /* X86_PAE */
#endif /* __LP64__ */

#ifndef __ASSEMBLER__

#include <kern/stdint.h>
#include <kern/types.h>
#include <machine/trap.h>

#ifdef X86_PAE
typedef uint64_t pmap_pte_t;
#else /* X86_PAE */
typedef unsigned long pmap_pte_t;
#endif /* X86_PAE */

/*
 * Physical address map.
 */
struct pmap {
    int __dummy;
};

/*
 * Early initialization of the MMU.
 *
 * This function is called before paging is enabled by the boot module. It
 * maps the kernel at physical and virtual addresses, after which all kernel
 * functions and data can be accessed.
 */
pmap_pte_t * pmap_setup_paging(void);

/*
 * This function is called by the AP bootstrap code before paging is enabled.
 */
pmap_pte_t * pmap_ap_setup_paging(void);

/*
 * Early initialization of the pmap module.
 */
void pmap_bootstrap(void);

/*
 * Early initialization of the MMU on APs.
 */
void pmap_ap_bootstrap(void);

/*
 * Allocate pure virtual memory.
 *
 * This memory is obtained from a very small pool of reserved pages located
 * immediately after the kernel. Its purpose is to allow early mappings to
 * be created before the VM system is available.
 */
unsigned long pmap_bootalloc(unsigned int nr_pages);

/*
 * Return the address below which using the kernel pmap functions is safe.
 *
 * Its value is adjusted by calling pmap_growkernel().
 */
unsigned long pmap_klimit(void);

/*
 * Preallocate resources so that addresses up to va can be mapped safely in
 * the kernel pmap.
 */
void pmap_growkernel(unsigned long va);

/*
 * Kernel specific mapping functions.
 *
 * Resources for the new mappings must be preallocated. The only function
 * which actually flushes the TLB is pmap_kupdate.
 */
void pmap_kenter(unsigned long va, phys_addr_t pa);
void pmap_kremove(unsigned long start, unsigned long end);
void pmap_kprotect(unsigned long start, unsigned long end, int prot);
void pmap_kupdate(unsigned long start, unsigned long end);
phys_addr_t pmap_kextract(unsigned long va);

/*
 * Interrupt handler for inter-processor update requests.
 */
void pmap_update_intr(struct trap_frame *frame);

#endif /* __ASSEMBLER__ */

#endif /* _X86_PMAP_H */
