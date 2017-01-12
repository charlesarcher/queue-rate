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
#ifndef _TIDEX_NPS_MUTEX_H_
#define _TIDEX_NPS_MUTEX_H_

#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <errno.h>

#define INVALID_TID  0
#define MAX_SPIN (1 << 10)

typedef struct
{
    atomic_long ingress;
    char padding1[64];      // To avoid false sharing
    atomic_long egress;
    char padding2[64];      // To avoid false sharing
    long nextEgress;
} tidex_nps_mutex_t;

/*
 * This variable can even be an 'atomic_short' because it is unlikely that your
 * application will create more than 32767 threads. This also means that
 * both ingress and egress can be of type 'atomic_short', which can save memory.
 *
 * We start at '1' because we want to use the negative of the value as well.
 * Alternatively, we could start at zero but then we would have to advance
 * this index 2 at a time.
 *
 * This is shared by all tidex_nps_mutex_t instances to save memory.
 */
static atomic_long globalThreadIndex = ATOMIC_VAR_INIT(1);

/*
 * The index of the thread is stored in a thread-local variable that is
 * shared by all instances of tidex_nps_mutex_t.
 * If the value is the initialized of INVALID_TID (zero) then we need to
 * get a value from globalThreadIndex using atomic_fetch_add(), once and
 * only once per thread.
 */
static _Thread_local long tlThreadIndex = INVALID_TID;

static inline void tidex_nps_mutex_init(tidex_nps_mutex_t * self);
static inline void tidex_nps_mutex_destroy(tidex_nps_mutex_t * self);
static inline void tidex_nps_mutex_lock(tidex_nps_mutex_t * self);
static inline void tidex_nps_mutex_unlock(tidex_nps_mutex_t * self);
static inline int tidex_nps_mutex_trylock(tidex_nps_mutex_t * self);

static inline void tidex_nps_mutex_init(tidex_nps_mutex_t * self)
{
    self->nextEgress = INVALID_TID;
    atomic_store(&self->ingress, INVALID_TID);
    atomic_store(&self->egress, INVALID_TID);
}


static inline void tidex_nps_mutex_destroy(tidex_nps_mutex_t * self)
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
static inline void tidex_nps_mutex_lock(tidex_nps_mutex_t * self)
{
    long mytid = tlThreadIndex;
    if (mytid == INVALID_TID) {
        tlThreadIndex = atomic_fetch_add(&globalThreadIndex, 1);
        mytid = tlThreadIndex;
    }
    if (atomic_load_explicit(&self->egress, memory_order_relaxed) == mytid) mytid = -mytid;
    long prevtid = atomic_exchange(&self->ingress, mytid);
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
static inline void tidex_nps_mutex_unlock(tidex_nps_mutex_t * self)
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
static inline int tidex_nps_mutex_trylock(tidex_nps_mutex_t * self)
{
    long localE = atomic_load(&self->egress);
    long localI = atomic_load(&self->ingress);
    if (localE != localI) return EBUSY;
    long mytid = tlThreadIndex;
    if (mytid == INVALID_TID) {
        tlThreadIndex = atomic_fetch_add(&globalThreadIndex, 1);
        mytid = tlThreadIndex;
    }
    if (localE == mytid) mytid = -mytid;
    if (!atomic_compare_exchange_strong(&self->ingress, &localI, mytid)) return EBUSY;
    // Lock has been acquired
    self->nextEgress = mytid;
    return 0;
}


#endif /* _TIDEX_NPS_MUTEX_H_ */
