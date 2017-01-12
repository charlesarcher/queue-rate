/******************************************************************************
 * Copyright (c) 2014-2015, Pedro Ramalhete, Andreia Correia
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
#ifndef _TICKET_ARRAY_WAITING_NODES_SPINS_BOTH_MUTEX_H_
#define _TICKET_ARRAY_WAITING_NODES_SPINS_BOTH_MUTEX_H_

#include <stdatomic.h>
#include <stdlib.h>
#include <sched.h>
#include <stdbool.h>
#include <errno.h>          // Needed by EBUSY

#define DEFAULT_MAX_WAITERS  8

typedef struct {
    atomic_bool lockIsMine;
} awnsb_node_t;

typedef struct
{
    atomic_llong ingress;
    char padding1[64];      // To avoid false sharing with the ingress and egress
    atomic_llong egress;
    char padding2[64];
    int maxArrayWaiters;
    awnsb_node_t ** waitersArray;
} ticket_awnsb_mutex_t;


void ticket_awnsb_mutex_init(ticket_awnsb_mutex_t * self, int maxArrayWaiters);
void ticket_awnsb_mutex_destroy(ticket_awnsb_mutex_t * self);
void ticket_awnsb_mutex_lock(ticket_awnsb_mutex_t * self);
void ticket_awnsb_mutex_unlock(ticket_awnsb_mutex_t * self);
int ticket_awnsb_mutex_trylock(ticket_awnsb_mutex_t * self);

#endif /* _TICKET_ARRAY_WAITING_NODES_SPINS_BOTH_MUTEX_H_ */
