/******************************************************************************
 * Copyright (c) 2014, Pedro Ramalhete, Andreia Correia
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Concurrency Freaks nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 */
#ifndef _TICKET_MUTEX_H_
#define _TICKET_MUTEX_H_

#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>

typedef struct
{
    _Atomic long ingress;
    char padding[64];      // To avoid false sharing with the head and tail
    _Atomic long egress;
} ticket_mutex_t;


static inline void ticket_mutex_init(ticket_mutex_t * self);
static inline void ticket_mutex_destroy(ticket_mutex_t * self);
static inline void ticket_mutex_lock(ticket_mutex_t * self);
static inline void ticket_mutex_unlock(ticket_mutex_t * self);

static inline void ticket_mutex_init(ticket_mutex_t * self)
{
    atomic_store(&self->ingress, 0);
    atomic_store(&self->egress, 0);
}


static inline void ticket_mutex_destroy(ticket_mutex_t * self)
{
    // kind of unnecessary, but oh well
    atomic_store(&self->ingress, 0);
    atomic_store(&self->egress, 0);
}


/*
 * Locks the mutex
 * Progress Condition: Blocking
 */
static inline void ticket_mutex_lock(ticket_mutex_t * self)
{
    long lingress = atomic_fetch_add(&self->ingress, 1);
    while (lingress != atomic_load(&self->egress)) {
        sched_yield();  // Replace this with thrd_yield() if you use <threads.h>
    }
    // This thread has acquired the lock on the mutex
}


/*
 * Unlocks the mutex
 * Progress Condition: Wait-Free Population Oblivious
 *
 * We could do a simple atomic_fetch_add(egress, 1) but it is faster to do
 * the relaxed load followed by the store with release barrier.
 * Notice that the load can be relaxed because the thread did an acquire
 * barrier when it read the "ingress" with the atomic_fetch_add() back in
 * ticket_mutex_lock() (or the acquire on reading "egress" at a second try),
 * and we have the guarantee that "egress" has not changed since then.
 */
static inline void ticket_mutex_unlock(ticket_mutex_t * self)
{
    long legress = atomic_load_explicit(&self->egress, memory_order_relaxed);
    atomic_store(&self->egress, legress+1);
}



#endif /* _TICKET_MUTEX_H_ */
