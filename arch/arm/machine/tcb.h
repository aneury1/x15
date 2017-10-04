/*
 * Copyright (c) 2017 Richard Braun.
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
 * Thread control block.
 */

#ifndef _ARM_TCB_H
#define _ARM_TCB_H

#include <stdnoreturn.h>

#include <kern/init.h>
#include <machine/page.h>

/*
 * Thread stack size.
 */
#define TCB_STACK_SIZE PAGE_SIZE

struct tcb {
};

int tcb_build(struct tcb *tcb, void *stack, void (*fn)(void *), void *arg);

void tcb_cleanup(struct tcb *tcb);

static inline struct tcb *
tcb_current(void)
{
    return NULL;
}

static inline void
tcb_set_current(struct tcb *tcb)
{
    (void)tcb;
}

noreturn void tcb_load(struct tcb *tcb);

static inline void
tcb_switch(struct tcb *prev, struct tcb *next)
{
    (void)prev;
    (void)next;
}

/*
 * This init operation provides :
 *  - current TCB handling
 */
INIT_OP_DECLARE(tcb_setup);

#endif /* _ARM_TCB_H */