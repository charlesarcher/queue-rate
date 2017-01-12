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
#ifndef _CLH_RWLOCK_H_
#define _CLH_RWLOCK_H_

#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>


typedef struct clh_rwlock_node_ clh_rwlock_node_t;

struct clh_rwlock_node_
{
    _Atomic char succ_must_wait;
};

typedef struct
{
    clh_rwlock_node_t * mynode;
    char padding1[64];  // To avoid false sharing with the tail
    _Atomic (clh_rwlock_node_t *) tail;
    char padding2[64];  // No point in having false-sharing with the tail
    _Atomic long readers_counter;
} clh_rwlock_t;


void clh_rwlock_init(clh_rwlock_t * self);
void clh_rwlock_destroy(clh_rwlock_t * self);
void clh_rwlock_readlock(clh_rwlock_t * self);
void clh_rwlock_readunlock(clh_rwlock_t * self);
void clh_rwlock_writelock(clh_rwlock_t * self);
void clh_rwlock_writeunlock(clh_rwlock_t * self);

#endif /* _CLH_RWLOCK_H_ */
