/*
 * Copyright (c) 2012 Richard Braun.
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
 */

#include <kern/assert.h>
#include <kern/init.h>
#include <kern/macros.h>
#include <kern/panic.h>
#include <kern/printk.h>
#include <kern/thread.h>
#include <machine/cpu.h>
#include <machine/lapic.h>
#include <machine/pic.h>
#include <machine/pmap.h>
#include <machine/trap.h>

/*
 * Type for interrupt service routines and trap handler functions.
 */
typedef void (*trap_isr_fn_t)(void);
typedef void (*trap_handler_fn_t)(struct trap_frame *);

/*
 * Properties of a trap handler.
 */
struct trap_handler {
    trap_handler_fn_t fn;
};

/*
 * Low level interrupt service routines.
 */
void trap_isr_default(void);
void trap_isr_divide_error(void);
void trap_isr_debug(void);
void trap_isr_nmi(void);
void trap_isr_breakpoint(void);
void trap_isr_overflow(void);
void trap_isr_bound_range(void);
void trap_isr_invalid_opcode(void);
void trap_isr_device_not_available(void);
void trap_isr_double_fault(void);
void trap_isr_invalid_tss(void);
void trap_isr_segment_not_present(void);
void trap_isr_stack_segment_fault(void);
void trap_isr_general_protection(void);
void trap_isr_page_fault(void);
void trap_isr_math_fault(void);
void trap_isr_alignment_check(void);
void trap_isr_machine_check(void);
void trap_isr_simd_fp_exception(void);
void trap_isr_pic_int7(void);
void trap_isr_pic_int15(void);
void trap_isr_pmap_update(void);
void trap_isr_lapic_timer(void);
void trap_isr_lapic_error(void);
void trap_isr_lapic_spurious(void);

/*
 * Array of trap handlers.
 *
 * The additional entry is the default entry used for unhandled traps.
 */
static struct trap_handler trap_handlers[CPU_IDT_SIZE + 1];

static void __init
trap_handler_init(struct trap_handler *handler, trap_handler_fn_t fn)
{
    handler->fn = fn;
}

static void __init
trap_install(unsigned int vector, trap_isr_fn_t isr, trap_handler_fn_t fn)
{
    assert(vector < CPU_IDT_SIZE);

    trap_handler_init(&trap_handlers[vector], fn);
    cpu_idt_set_gate(vector, isr);
}

static void
trap_double_fault(struct trap_frame *frame)
{
#ifndef __LP64__
    struct trap_frame frame_store;
    struct cpu *cpu;

    /*
     * Double faults are catched through a task gate, which makes the given
     * frame useless. The interrupted state is automatically saved in the
     * main TSS by the processor. Build a proper trap frame from there.
     */
    frame = &frame_store;
    cpu = cpu_current();
    frame->eax = cpu->tss.eax;
    frame->ebx = cpu->tss.ebx;
    frame->ecx = cpu->tss.ecx;
    frame->edx = cpu->tss.edx;
    frame->ebp = cpu->tss.ebp;
    frame->esi = cpu->tss.esi;
    frame->edi = cpu->tss.edi;
    frame->ds = cpu->tss.ds;
    frame->es = cpu->tss.es;
    frame->fs = cpu->tss.fs;
    frame->gs = cpu->tss.gs;
    frame->vector = TRAP_DF;
    frame->error = 0;
    frame->eip = cpu->tss.eip;
    frame->cs = cpu->tss.cs;
    frame->eflags = cpu->tss.eflags;
    frame->esp = cpu->tss.esp;
    frame->ss = cpu->tss.ss;
#endif /* __LP64__ */

    printk("trap: double fault:\n");
    trap_frame_show(frame);
    cpu_halt();
}

static void __init
trap_install_double_fault(void)
{
    trap_handler_init(&trap_handlers[TRAP_DF], trap_double_fault);
    cpu_idt_set_double_fault(trap_isr_double_fault);
}

static void
trap_default(struct trap_frame *frame)
{
    printk("trap: unhandled interrupt or exception:\n");
    trap_frame_show(frame);

    cpu_intr_disable();

    for (;;)
        cpu_idle();
}

void __init
trap_setup(void)
{
    size_t i;

    for (i = 0; i < CPU_IDT_SIZE; i++)
        trap_install(i, trap_isr_default, trap_default);

    /* Architecture defined traps */
    trap_install(TRAP_DE, trap_isr_divide_error, trap_default);
    trap_install(TRAP_DB, trap_isr_debug, trap_default);
    trap_install(TRAP_NMI, trap_isr_nmi, trap_default);
    trap_install(TRAP_BP, trap_isr_breakpoint, trap_default);
    trap_install(TRAP_OF, trap_isr_overflow, trap_default);
    trap_install(TRAP_BR, trap_isr_bound_range, trap_default);
    trap_install(TRAP_UD, trap_isr_invalid_opcode, trap_default);
    trap_install(TRAP_NM, trap_isr_device_not_available, trap_default);
    trap_install_double_fault();
    trap_install(TRAP_TS, trap_isr_invalid_tss, trap_default);
    trap_install(TRAP_NP, trap_isr_segment_not_present, trap_default);
    trap_install(TRAP_SS, trap_isr_stack_segment_fault, trap_default);
    trap_install(TRAP_GP, trap_isr_general_protection, trap_default);
    trap_install(TRAP_PF, trap_isr_page_fault, trap_default);
    trap_install(TRAP_MF, trap_isr_math_fault, trap_default);
    trap_install(TRAP_AC, trap_isr_alignment_check, trap_default);
    trap_install(TRAP_MC, trap_isr_machine_check, trap_default);
    trap_install(TRAP_XM, trap_isr_simd_fp_exception, trap_default);

    /* Basic PIC support */
    trap_install(TRAP_PIC_BASE + 7, trap_isr_pic_int7, pic_intr_spurious);
    trap_install(TRAP_PIC_BASE + 15, trap_isr_pic_int15, pic_intr_spurious);

    /* System defined traps */
    trap_install(TRAP_PMAP_UPDATE, trap_isr_pmap_update, pmap_update_intr);
    trap_install(TRAP_LAPIC_TIMER, trap_isr_lapic_timer, lapic_intr_timer);
    trap_install(TRAP_LAPIC_ERROR, trap_isr_lapic_error, lapic_intr_error);
    trap_install(TRAP_LAPIC_SPURIOUS, trap_isr_lapic_spurious,
                 lapic_intr_spurious);

    trap_handler_init(&trap_handlers[TRAP_DEFAULT], trap_default);
}

void
trap_main(struct trap_frame *frame)
{
    assert(frame->vector < ARRAY_SIZE(trap_handlers));
    trap_handlers[frame->vector].fn(frame);
    thread_intr_schedule();
}

#ifdef __LP64__

void
trap_frame_show(struct trap_frame *frame)
{
    printk("trap: rax: %016lx rbx: %016lx rcx: %016lx\n"
           "trap: rdx: %016lx rbp: %016lx rsi: %016lx\n"
           "trap: rdi: %016lx  r8: %016lx  r9: %016lx\n"
           "trap: r10: %016lx r11: %016lx r12: %016lx\n"
           "trap: r13: %016lx r14: %016lx r15: %016lx\n"
           "trap: vector: %lu error: %08lx\n"
           "trap: rip: %016lx cs: %lu rflags: %016lx\n"
           "trap: rsp: %016lx ss: %lu\n",
           frame->rax, frame->rbx, frame->rcx, frame->rdx, frame->rbp,
           frame->rsi, frame->rdi, frame->r8, frame->r9, frame->r10,
           frame->r11, frame->r12, frame->r13, frame->r14, frame->r15,
           frame->vector, frame->error, frame->rip, frame->cs, frame->rflags,
           frame->rsp, frame->ss);
}

#else /* __LP64__ */

void
trap_frame_show(struct trap_frame *frame)
{
    unsigned long esp, ss;

    if ((frame->cs & CPU_PL_USER) || (frame->vector == TRAP_DF)) {
        esp = frame->esp;
        ss = frame->ss;
    } else {
        esp = 0;
        ss = 0;
    }

    printk("trap: eax: %08lx ebx: %08lx ecx: %08lx edx: %08lx\n"
           "trap: ebp: %08lx esi: %08lx edi: %08lx\n"
           "trap: ds: %lu es: %lu fs: %lu gs: %lu\n"
           "trap: vector: %lu error: %08lx\n"
           "trap: eip: %08lx cs: %lu eflags: %08lx\n"
           "trap: esp: %08lx ss: %lu\n",
           frame->eax, frame->ebx, frame->ecx, frame->edx, frame->ebp,
           frame->esi, frame->edi, frame->ds, frame->es, frame->fs, frame->gs,
           frame->vector, frame->error, frame->eip, frame->cs, frame->eflags,
           esp, ss);
}

#endif /* __LP64__ */
