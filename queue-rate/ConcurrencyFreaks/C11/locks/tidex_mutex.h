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
#ifndef _TIDEX_MUTEX_H_
#define _TIDEX_MUTEX_H_

#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>

#define INVALID_TID  0
#define MAX_SPIN (1 << 10)

/* We're assuming that 'long long' is enough to hold a pthread_t */
typedef struct
{
    _Atomic long long ingress;
    char padding1[64];      // To avoid false sharing
    _Atomic long long egress;
    char padding2[64];      // To avoid false sharing
    long long nextEgress;
} tidex_mutex_t;


static inline void tidex_mutex_init(tidex_mutex_t * self);
static inline void tidex_mutex_destroy(tidex_mutex_t * self);
static inline void tidex_mutex_lock(tidex_mutex_t * self);
static inline void tidex_mutex_unlock(tidex_mutex_t * self);
int tidex_mutex_trylock(tidex_mutex_t * self);


static inline void tidex_mutex_init(tidex_mutex_t * self)
{
    self->nextEgress = INVALID_TID;
    atomic_store(&self->ingress, INVALID_TID);
    atomic_store(&self->egress, INVALID_TID);
}


static inline void tidex_mutex_destroy(tidex_mutex_t * self)
{
    // Kind of unnecessary, but oh well
    atomic_store(&self->ingress, INVALID_TID);
    atomic_store(&self->egress, INVALID_TID);
}


/*
 * Locks the mutex
 * Progress Condition: Blocking
 *
 * The first load on egress can be relaxed because we're only interested
 * in finding out whether it is the same thread id as the current thread or
 * not. If it it is the same, then it is guaranteed to be up-to-date, and if
 * it is different we don't care. It can also 'seem' the same and no longer
 * be the same, which is also ok because we'll be using the negative of
 * pthread_self() when could in fact use pthread_self(), but that's not
 * a problem.
 */
static inline void tidex_mutex_lock(tidex_mutex_t * self)
{
    long long mytid = (long long)pthread_self();
    if (atomic_load_explicit(&self->egress, memory_order_relaxed) == mytid) mytid = -mytid;
    long long prevtid = atomic_exchange(&self->ingress, mytid);
    while (atomic_load(&self->egress) != prevtid) {
        // Spin for a while and then yield
        for (int k = MAX_SPIN; k > 0; k--) {
            if (atomic_load(&self->egress) == prevtid) {
                // Lock has been acquired
                self->nextEgress = mytid;
                return;
            }
        }
        sched_yield();  // Replace this with thrd_yield() if you use <threads.h>
    }
    // Lock has been acquired
    self->nextEgress = mytid;
}


/*
 * Unlocks the mutex
 * Progress Condition: Wait-Free Population Oblivious
 */
static inline void tidex_mutex_unlock(tidex_mutex_t * self)
{
    atomic_store(&self->egress, self->nextEgress);
}


/*
 * Tries to lock the mutex
 * Returns 0 if the lock has been acquired and EBUSY otherwise
 * Progress Condition: Wait-Free Population Oblivious
 *
 * Yes, we must use a CAS instead of an EXCHG, but it's
 * still wait-free because if the CAS fails we give up (there
 * is already another thread holding the lock).
 */
int tidex_mutex_trylock(tidex_mutex_t * self)
{
    long long localE = atomic_load(&self->egress);
    long long localI = atomic_load_explicit(&self->ingress, memory_order_relaxed);
    if (localE != localI) return EBUSY;
    long long mytid = (long long)pthread_self();
    if (localE == mytid) mytid = -mytid;
    if (!atomic_compare_exchange_strong(&self->ingress, &localI, mytid)) return EBUSY;
    // Lock has been acquired
    self->nextEgress = mytid;
    return 0;
}


#endif /* _TIDEX_MUTEX_H_ */
