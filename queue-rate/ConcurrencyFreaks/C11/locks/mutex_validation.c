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

/*
 * This file can be compiled with something like (you'll need gcc 4.9.x):
 * gcc --std=c11 -D_XOPEN_SOURCE=600 mutex_validation.c mpsc_mutex.c ticket_mutex.c clh_mutex.c -lpthread -o mbench
 * Feel free to add  -O3 -march=native
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>      /* Needed by sleep() */
#include <time.h>        /* Needed by rand()/srand() */
#include "mpsc_mutex.h"
#include "ticket_mutex.h"
#include "clh_mutex.h"
#include "ticketawn/ticket_awnne_mutex.h"
#include "ticketawn/ticket_awnee_mutex.h"
#include "ticketawn/ticket_awnsb_mutex.h"
#include "tidex_mutex.h"
#include "tidex_nps_mutex.h"


/*
 * Benchmark parameters
 */
#define ARRAY_SIZE   (256)
#define NUM_THREADS  4

/*
 * Global variables
 */
int *array1;

pthread_mutex_t pmutex;
pthread_spinlock_t pspin;
mpsc_mutex_t mpscmutex;
ticket_mutex_t ticketmutex;
clh_mutex_t clhmutex;
tidex_mutex_t tidexmutex;
tidex_nps_mutex_t tidexnpsmutex;
ticket_awnne_mutex_t ticketawnnemutex;
ticket_awnee_mutex_t ticketawneemutex;
ticket_awnsb_mutex_t ticketawnsbmutex;

#define TYPE_PTHREAD_MUTEX       0
#define TYPE_PTHREAD_SPIN        1
#define TYPE_MPSC_MUTEX          2
#define TYPE_TICKET_MUTEX        3
#define TYPE_CLH_MUTEX           4
#define TYPE_TIDEX_MUTEX         5
#define TYPE_TIDEX_NPS_MUTEX     6
#define TYPE_TICKET_AWNNE_MUTEX  7
#define TYPE_TICKET_AWNEE_MUTEX  8
#define TYPE_TICKET_AWNSB_MUTEX  9

int g_which_lock = TYPE_PTHREAD_MUTEX;
int g_quit = 0;
int g_operCounters[NUM_THREADS];


static void clearOperCounters(void) {
    int i;
    for (i = 0; i < NUM_THREADS; i++) g_operCounters[i] = 0;
}

static void printOperationsPerSecond() {
    int i;
    long sum = 0;
    for (i = 0; i < NUM_THREADS; i++) sum += g_operCounters[i];
    printf("Operations/sec = %d\n", sum);
}

/**
 *
 */
void worker_thread(int *tid) {
    int i;
    int *current_array;
    long iterations = 0;

    while (!g_quit) {
        if (g_which_lock == TYPE_PTHREAD_MUTEX) {
            /* Critical path for pthread_rwlock_t */
            pthread_mutex_lock(&pmutex);
            for (i = 0; i < ARRAY_SIZE; i++) array1[i]++;
            for (i = 1; i < ARRAY_SIZE; i++) {
                if (array1[i] != array1[0]) printf("ERROR\n");
            }
            pthread_mutex_unlock(&pmutex);
        } else if (g_which_lock == TYPE_PTHREAD_SPIN) {
            /* Critical path for pthread_spin_t */
            pthread_spin_lock(&pspin);
            for (i = 0; i < ARRAY_SIZE; i++) array1[i]++;
            for (i = 1; i < ARRAY_SIZE; i++) {
                if (array1[i] != array1[0]) printf("ERROR\n");
            }
            pthread_spin_unlock(&pspin);
        } else if (g_which_lock == TYPE_MPSC_MUTEX) {
            /* Critical path for mpsc_mutex_t */
            mpsc_mutex_lock(&mpscmutex);
            for (i = 0; i < ARRAY_SIZE; i++) array1[i]++;
            for (i = 1; i < ARRAY_SIZE; i++) {
                if (array1[i] != array1[0]) printf("ERROR\n");
            }
            mpsc_mutex_unlock(&mpscmutex);
        } else if (g_which_lock == TYPE_TICKET_MUTEX) {
            /* Critical path for ticket_mutex_t */
            ticket_mutex_lock(&ticketmutex);
            for (i = 0; i < ARRAY_SIZE; i++) array1[i]++;
            for (i = 1; i < ARRAY_SIZE; i++) {
                if (array1[i] != array1[0]) printf("ERROR\n");
            }
            ticket_mutex_unlock(&ticketmutex);
        } else if (g_which_lock == TYPE_CLH_MUTEX){
            /* Critical path for clh_mutex_t */
            clh_mutex_lock(&clhmutex);
            for (i = 0; i < ARRAY_SIZE; i++) array1[i]++;
            for (i = 1; i < ARRAY_SIZE; i++) {
                if (array1[i] != array1[0]) printf("ERROR\n");
            }
            clh_mutex_unlock(&clhmutex);
        } else  if (g_which_lock == TYPE_TIDEX_MUTEX) {
            /* Critical path for tidex_mutex_t */
            tidex_mutex_lock(&tidexmutex);
            for (i = 0; i < ARRAY_SIZE; i++) array1[i]++;
            for (i = 1; i < ARRAY_SIZE; i++) {
                if (array1[i] != array1[0]) printf("ERROR\n");
            }
            tidex_mutex_unlock(&tidexmutex);
        } else if (g_which_lock == TYPE_TIDEX_NPS_MUTEX) {
            /* Critical path for tidex_nps_mutex_t */
            tidex_nps_mutex_lock(&tidexnpsmutex);
            for (i = 0; i < ARRAY_SIZE; i++) array1[i]++;
            for (i = 1; i < ARRAY_SIZE; i++) {
                if (array1[i] != array1[0]) printf("ERROR\n");
            }
            tidex_nps_mutex_unlock(&tidexnpsmutex);
        } else if (g_which_lock == TYPE_TICKET_AWNNE_MUTEX) {
            /* Critical path for ticket_awnne_mutex_t */
            ticket_awnne_mutex_lock(&ticketawnnemutex);
            for (i = 0; i < ARRAY_SIZE; i++) array1[i]++;
            for (i = 1; i < ARRAY_SIZE; i++) {
                if (array1[i] != array1[0]) printf("ERROR\n");
            }
            ticket_awnne_mutex_unlock(&ticketawnnemutex);
        } else if (g_which_lock == TYPE_TICKET_AWNEE_MUTEX) {
            /* Critical path for ticket_awnee_mutex_t */
            ticket_awnee_mutex_lock(&ticketawneemutex);
            for (i = 0; i < ARRAY_SIZE; i++) array1[i]++;
            for (i = 1; i < ARRAY_SIZE; i++) {
                if (array1[i] != array1[0]) printf("ERROR\n");
            }
            ticket_awnee_mutex_unlock(&ticketawneemutex);
        } else {
            /* Critical path for ticket_awnsb_mutex_t */
            ticket_awnsb_mutex_lock(&ticketawnsbmutex);
            for (i = 0; i < ARRAY_SIZE; i++) array1[i]++;
            for (i = 1; i < ARRAY_SIZE; i++) {
                if (array1[i] != array1[0]) printf("ERROR\n");
            }
            ticket_awnsb_mutex_unlock(&ticketawnsbmutex);
        }
        iterations++;
    }

    printf("Thread %d, iterations = %ld\n", *tid, iterations);
    g_operCounters[*tid] = iterations;
}


/**
 * Starts 4 pthreads and uses either a pthread_rwlock_t or a di_rwlock_t
 * to protect access to an array.
 *
 */
int main(void) {
    int i;
    pthread_t *pthread_list;
    int threadid[NUM_THREADS];

    /* Allocate memory for the two instance arrays */
    array1 = (int *)malloc(ARRAY_SIZE*sizeof(int));
    if (array1 == NULL) {
        printf("Not enough memory to allocate array\n");
        return -1;
    }
    for (i = 0; i < ARRAY_SIZE; i++) {
        array1[i] = 0;
    }

    /* Initialize locks */
    pthread_mutex_init(&pmutex, NULL);
    pthread_spin_init(&pspin, PTHREAD_PROCESS_PRIVATE);
    mpsc_mutex_init(&mpscmutex);
    ticket_mutex_init(&ticketmutex);
    clh_mutex_init(&clhmutex);
    tidex_mutex_init(&tidexmutex);
    tidex_nps_mutex_init(&tidexnpsmutex);
    ticket_awnne_mutex_init(&ticketawnnemutex, 34);
    ticket_awnee_mutex_init(&ticketawneemutex, 34);
    ticket_awnsb_mutex_init(&ticketawnsbmutex, 34);

    printf("Starting benchmark with %d threads\n", NUM_THREADS);
    printf("Array has size of %d\n", ARRAY_SIZE);

    // Create the threads
    pthread_list = (pthread_t *)calloc(sizeof(pthread_t), NUM_THREADS);

    printf("pthread_mutex_t, sleeping for 10 seconds...\n");
    g_which_lock = TYPE_PTHREAD_MUTEX;
    clearOperCounters();
    // Start the threads
    for(i = 0; i < NUM_THREADS; i++ ) {
        threadid[i] = i;
        pthread_create(&pthread_list[i], NULL, (void *(*)(void *))worker_thread, (void *)&threadid[i]);
    }
    sleep(10);
    g_quit = 1;

    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(pthread_list[i], NULL);
    }
    g_quit = 0;
    printOperationsPerSecond();

    printf("pthread_spin_t, sleeping for 10 seconds\n");
    g_which_lock = TYPE_PTHREAD_SPIN;
    clearOperCounters();
    for(i = 0; i < NUM_THREADS; i++ ) {
        pthread_create(&pthread_list[i], NULL, (void *(*)(void *))worker_thread, (void *)&threadid[i]);
    }
    sleep(10);
    g_quit = 1;
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(pthread_list[i], NULL);
    }
    g_quit = 0;
    printOperationsPerSecond();

    printf("mpsc_mutex_t, sleeping for 10 seconds\n");
    g_which_lock = TYPE_MPSC_MUTEX;
    clearOperCounters();
    for(i = 0; i < NUM_THREADS; i++ ) {
        pthread_create(&pthread_list[i], NULL, (void *(*)(void *))worker_thread, (void *)&threadid[i]);
    }
    sleep(10);
    g_quit = 1;
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(pthread_list[i], NULL);
    }
    g_quit = 0;
    printOperationsPerSecond();

    printf("ticket_mutex_t, sleeping for 10 seconds...\n");
    g_which_lock = TYPE_TICKET_MUTEX;
    clearOperCounters();
    // Start the threads
    for(i = 0; i < NUM_THREADS; i++ ) {
        threadid[i] = i;
        pthread_create(&pthread_list[i], NULL, (void *(*)(void *))worker_thread, (void *)&threadid[i]);
    }
    sleep(10);
    g_quit = 1;
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(pthread_list[i], NULL);
    }
    g_quit = 0;
    printOperationsPerSecond();

    printf("clh_mutex_t, sleeping for 10 seconds...\n");
    g_which_lock = TYPE_CLH_MUTEX;
    clearOperCounters();
    // Start the threads
    for(i = 0; i < NUM_THREADS; i++ ) {
        threadid[i] = i;
        pthread_create(&pthread_list[i], NULL, (void *(*)(void *))worker_thread, (void *)&threadid[i]);
    }
    sleep(10);
    g_quit = 1;
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(pthread_list[i], NULL);
    }
    g_quit = 0;
    printOperationsPerSecond();

    printf("tidex_mutex_t, sleeping for 10 seconds...\n");
    g_which_lock = TYPE_TIDEX_MUTEX;
    clearOperCounters();
    // Start the threads
    for(i = 0; i < NUM_THREADS; i++ ) {
        threadid[i] = i;
        pthread_create(&pthread_list[i], NULL, (void *(*)(void *))worker_thread, (void *)&threadid[i]);
    }
    sleep(10);
    g_quit = 1;
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(pthread_list[i], NULL);
    }
    g_quit = 0;
    printOperationsPerSecond();

    printf("tidex_nps_mutex_t, sleeping for 10 seconds...\n");
    g_which_lock = TYPE_TIDEX_NPS_MUTEX;
    clearOperCounters();
    // Start the threads
    for(i = 0; i < NUM_THREADS; i++ ) {
        threadid[i] = i;
        pthread_create(&pthread_list[i], NULL, (void *(*)(void *))worker_thread, (void *)&threadid[i]);
    }
    sleep(10);
    g_quit = 1;
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(pthread_list[i], NULL);
    }
    g_quit = 0;
    printOperationsPerSecond();

    printf("ticket_awnne_mutex_t (Negative Egress), sleeping for 10 seconds...\n");
    g_which_lock = TYPE_TICKET_AWNNE_MUTEX;
    clearOperCounters();
    // Start the threads
    for(i = 0; i < NUM_THREADS; i++ ) {
        threadid[i] = i;
        pthread_create(&pthread_list[i], NULL, (void *(*)(void *))worker_thread, (void *)&threadid[i]);
    }
    sleep(10);
    g_quit = 1;
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(pthread_list[i], NULL);
    }
    g_quit = 0;
    printOperationsPerSecond();

    printf("ticket_awnee_mutex_t (Ends on Egress), sleeping for 10 seconds...\n");
    g_which_lock = TYPE_TICKET_AWNEE_MUTEX;
    clearOperCounters();
    // Start the threads
    for(i = 0; i < NUM_THREADS; i++ ) {
        threadid[i] = i;
        pthread_create(&pthread_list[i], NULL, (void *(*)(void *))worker_thread, (void *)&threadid[i]);
    }
    sleep(10);
    g_quit = 1;
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(pthread_list[i], NULL);
    }
    g_quit = 0;
    printOperationsPerSecond();

    printf("ticket_awnsb_mutex_t (Spins on Both), sleeping for 10 seconds...\n");
    g_which_lock = TYPE_TICKET_AWNSB_MUTEX;
    clearOperCounters();
    // Start the threads
    for(i = 0; i < NUM_THREADS; i++ ) {
        threadid[i] = i;
        pthread_create(&pthread_list[i], NULL, (void *(*)(void *))worker_thread, (void *)&threadid[i]);
    }
    sleep(10);
    g_quit = 1;
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(pthread_list[i], NULL);
    }
    g_quit = 0;
    printOperationsPerSecond();


    /* Destroy locks */
    pthread_mutex_destroy(&pmutex);
    pthread_spin_destroy(&pspin);
    mpsc_mutex_destroy(&mpscmutex);
    ticket_mutex_destroy(&ticketmutex);
    clh_mutex_destroy(&clhmutex);
    tidex_mutex_destroy(&tidexmutex);
    tidex_nps_mutex_destroy(&tidexnpsmutex);
    ticket_awnne_mutex_destroy(&ticketawnnemutex);
    ticket_awnee_mutex_destroy(&ticketawneemutex);
    ticket_awnsb_mutex_destroy(&ticketawnsbmutex);

    /* Release memory for the array instances and threads */
    free(array1);
    free(pthread_list);
    return 0;
}
