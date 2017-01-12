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
#ifndef _MPSC_MUTEX_H_
#define _MPSC_MUTEX_H_

#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>

typedef struct mpsc_mutex_node_ mpsc_mutex_node_t;

struct mpsc_mutex_node_
{
    _Atomic (mpsc_mutex_node_t *) next;
};


typedef struct
{
    _Atomic (mpsc_mutex_node_t *) head;
    char padding[64];                     // To avoid false sharing with the head and tail
    _Atomic (mpsc_mutex_node_t *) tail;
} mpsc_mutex_t;


static inline void mpsc_mutex_init(mpsc_mutex_t * self);
static inline void mpsc_mutex_destroy(mpsc_mutex_t * self);
static inline void mpsc_mutex_lock(mpsc_mutex_t * self);
static inline void mpsc_mutex_unlock(mpsc_mutex_t * self);


static mpsc_mutex_node_t * mpsc_mutex_create_node(void)
{
    mpsc_mutex_node_t * new_node = (mpsc_mutex_node_t *)malloc(sizeof(mpsc_mutex_node_t));
    return new_node;
}


static inline void mpsc_mutex_init(mpsc_mutex_t * self)
{
    mpsc_mutex_node_t * node = mpsc_mutex_create_node();
    atomic_store(&self->head, node);
    atomic_store(&self->tail, node);
}


static inline void mpsc_mutex_destroy(mpsc_mutex_t * self)
{
    // TODO: check if head and tail are equal (no other thread waiting on the lock)
    free(atomic_load(&self->head));
}


/*
 * 1. A thread wishing to acquire the lock starts by creating a new node that
 *    it will insert in the tail of the queue, using an atomic_exchange() on
 *    the tail.
 * 2. The atomic_exchange() operation will return a pointer to the previous
 *    node to which tail was pointing, and now this thread can set the "next"
 *    of that node to point to its own node, using an atomic_store().
 * 3. We now loop until the head reaches the node previous to our own.
 */
static inline void mpsc_mutex_lock(mpsc_mutex_t * self)
{
    mpsc_mutex_node_t *mynode = mpsc_mutex_create_node();
    mpsc_mutex_node_t *prev = atomic_exchange(&self->tail, mynode);
    atomic_store(&prev->next, mynode);

    // This thread's node is now in the queue, so wait until it is its turn
    mpsc_mutex_node_t * lhead = atomic_load(&self->head);
    while (lhead != prev) {
        sched_yield();  // Replace this with thrd_yield() if you use <threads.h>
        lhead = atomic_load(&self->head);
    }
    // This thread has acquired the lock on the mutex
}


/*
 * 1. We assume that if unlock() is being called, it is because the current
 *    thread is holding the lock, which means that the node to which "head"
 *    points to is the one previous to the node created by the current thread,
 *    so now all that needs to be done is to advance the head to the next node
 *    and free() the memory of the previous which is now inaccessible, and
 *    its "next" field will never be de-referenced by any other thread.
 */
static inline void mpsc_mutex_unlock(mpsc_mutex_t * self)
{
    // We assume that if this function was called is because this thread is
    // currently holding the lock, which means that the head->next is mynode
    mpsc_mutex_node_t * prev = atomic_load_explicit(&self->head, memory_order_relaxed);
    mpsc_mutex_node_t * mynode = atomic_load_explicit(&prev->next, memory_order_relaxed);

    if (mynode == NULL) {
        // TODO: too many unlocks ???
        return;
    }
    atomic_store(&self->head, mynode);
    free(prev);
}



#endif /* _MPSC_MUTEX_H_ */
