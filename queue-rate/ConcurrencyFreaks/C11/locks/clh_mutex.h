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
#ifndef _CLH_MUTEX_H_
#define _CLH_MUTEX_H_

#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>


typedef struct clh_mutex_node_ clh_mutex_node_t;

struct clh_mutex_node_
{
    _Atomic char succ_must_wait;
};

typedef struct
{
    clh_mutex_node_t * mynode;
    char padding[64];  // To avoid false sharing with the tail
    _Atomic (clh_mutex_node_t *) tail;
} clh_mutex_t;


static inline void clh_mutex_init(clh_mutex_t * self);
static inline void clh_mutex_destroy(clh_mutex_t * self);
static inline void clh_mutex_lock(clh_mutex_t * self);
static inline void clh_mutex_unlock(clh_mutex_t * self);

static clh_mutex_node_t * clh_mutex_create_node(char islocked)
{
    clh_mutex_node_t * new_node = (clh_mutex_node_t *)malloc(sizeof(clh_mutex_node_t));
    atomic_store_explicit(&new_node->succ_must_wait, islocked, memory_order_relaxed);
    return new_node;
}


/*
 * Initializes the mutex, creating a sentinel node.
 *
 * Progress Condition: Wait-Free Population Oblivious
 */
static inline void clh_mutex_init(clh_mutex_t * self)
{
    // We create the first sentinel node unlocked, with islocked=0
    clh_mutex_node_t * node = clh_mutex_create_node(0);
    self->mynode = node;
    atomic_store(&self->tail, node);
}


/*
 * Destroy the mutex, clearing all memory.
 * You must be sure that there are no other threads currently holding
 * the lock or attempting to.
 *
 * Progress Condition: Wait-Free Population Oblivious
 */
static inline void clh_mutex_destroy(clh_mutex_t * self)
{
    free(atomic_load(&self->tail));
}


/*
 * Locks the mutex for the current thread. Will wait for other threads
 * that did the atomic_exchange() before this one.
 *
 * Progress Condition: Blocking
 */
static inline void clh_mutex_lock(clh_mutex_t * self)
{
    // Create the new node locked by default, setting islocked=1
    clh_mutex_node_t *mynode = clh_mutex_create_node(1);
    clh_mutex_node_t *prev = atomic_exchange(&self->tail, mynode);

    // This thread's node is now in the queue, so wait until it is its turn
    char prev_islocked = atomic_load_explicit(&prev->succ_must_wait, memory_order_relaxed);
    if (prev_islocked) {
        while (prev_islocked) {
            sched_yield();  // Replace this with thrd_yield() if you use <threads.h>
            prev_islocked = atomic_load(&prev->succ_must_wait);
        }
    }
    // This thread has acquired the lock on the mutex and it is now safe to
    // cleanup the memory of the previous node.
    free(prev);

    // Store mynode for clh_mutex_unlock() to use. We could replace
    // this with a thread-local, not sure which is faster.
    self->mynode = mynode;
}


/*
 * Unlocks the mutex. Assumes that the current thread holds the lock on the
 * mutex.
 *
 * Progress Condition: Wait-Free Population Oblivious
 */
static inline void clh_mutex_unlock(clh_mutex_t * self)
{
    // We assume that if this function was called, it is because this thread is
    // currently holding the lock, which means that self->mynode is pointing to
    // the current thread's mynode.
    if (self->mynode == NULL) {
        // ERROR: This will occur if unlock() is called without a lock()
        return;
    }
    atomic_store(&self->mynode->succ_must_wait, 0);
}




#endif /* _CLH_MUTEX_H_ */
