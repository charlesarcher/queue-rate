// -*- mode: c; c-basic-offset:4 ; indent-tabs-mode:nil ; -*-
#define _GNU_SOURCE
#include <sched.h>
#include <hwloc.h>
#include <hwloc/glibc-sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <ctype.h>
#include "ConcurrencyFreaks/C11/locks/clh_mutex.h"
#include "ConcurrencyFreaks/C11/locks/mpsc_mutex.h"
#include "ConcurrencyFreaks/C11/locks/tidex_mutex.h"
#include "ConcurrencyFreaks/C11/locks/ticket_mutex.h"
#include "ConcurrencyFreaks/C11/locks/tidex_nps_mutex.h"

extern int printme(char *instr);
extern int urandom_init();
extern unsigned long urandom(int urandom_fd);

#if ( (__GNUC__ == 4) && (__GNUC_MINOR__ >= 1) || __GNUC__ > 4) &&  \
        (defined(__x86_64__) || defined(__i386__))
#define XCHG __sync_lock_test_and_set
#endif

/*#define DEBUG */
#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#else
#define DEBUG_PRINT(...) do{ } while ( 0 )
#endif

#define PRINT_BATCH 2
#define PRINT_AFFINITY 0

#define PTHREAD_LOCK     0
#define PTHREAD_SPINLOCK 1
#define CLH_LOCK         2
#define MPSC_LOCK        3
#define TIDEX_LOCK       4
#define TICKET_LOCK      5
#define TIDEX_NPS_LOCK   6

//#define LOCK_METHOD PTHREAD
//#define LOCK_METHOD PTHREAD_SPINLOCK
//#define LOCK_METHOD CLH_LOCK
//#define LOCK_METHOD MPSC_LOCK
//#define LOCK_METHOD TIDEX_LOCK
//#define LOCK_METHOD TICKET_LOCK
//#define LOCK_METHOD TIDEX_NPS_LOCK

pthread_barrier_t g_barrier;
hwloc_topology_t  g_topo;
pthread_mutex_t   g_mutex = PTHREAD_MUTEX_INITIALIZER;
int               g_done;
int               g_random_fd;

#if LOCK_METHOD==PTHREAD_LOCK
#define LOCK_PAD 64
#define MUTEX_NAME "Pthread Lock"
typedef struct lock_t {
    pthread_mutex_t mutex;
} lock_t;

static inline void lock_init(lock_t *lock)
{
    pthread_mutex_init(&lock->mutex,NULL);
}

static inline void lock(lock_t *lock)
{
    pthread_mutex_lock(&lock->mutex);

}

static inline void unlock(lock_t *lock)
{
    pthread_mutex_unlock(&lock->mutex);
}

#elif LOCK_METHOD==PTHREAD_SPINLOCK
#define LOCK_PAD 64
#define MUTEX_NAME "Pthread Spin Lock"
typedef struct lock_t {
    pthread_spinlock_t mutex;
} lock_t;

static inline void lock_init(lock_t *lock)
{
    pthread_spin_init(&lock->mutex,0);
}

static inline void lock(lock_t *lock)
{
    pthread_spin_lock(&lock->mutex);

}

static inline void unlock(lock_t *lock)
{
    pthread_spin_unlock(&lock->mutex);
}

#elif LOCK_METHOD==CLH_LOCK
#define LOCK_PAD 128
#define MUTEX_NAME "CLH Lock"
typedef struct lock_t {
    clh_mutex_t mutex;
} lock_t;

static inline void lock_init(lock_t *lock)
{
    clh_mutex_init(&lock->mutex);
}

static inline void lock(lock_t *lock)
{
    clh_mutex_lock(&lock->mutex);

}

static inline void unlock(lock_t *lock)
{
    clh_mutex_unlock(&lock->mutex);
}

#elif LOCK_METHOD==MPSC_LOCK
#define LOCK_PAD 128
#define MUTEX_NAME "MPSC Lock"
typedef struct lock_t {
    mpsc_mutex_t mutex;
} lock_t;

static inline void lock_init(lock_t *lock)
{
    mpsc_mutex_init(&lock->mutex);
}

static inline void lock(lock_t *lock)
{
    mpsc_mutex_lock(&lock->mutex);

}

static inline void unlock(lock_t *lock)
{
    mpsc_mutex_unlock(&lock->mutex);
}

#elif LOCK_METHOD==TIDEX_LOCK
#define LOCK_PAD 256
#define MUTEX_NAME "Tidex Lock"
typedef struct lock_t {
    tidex_mutex_t mutex;
} lock_t;

static inline void lock_init(lock_t *lock)
{
    tidex_mutex_init(&lock->mutex);
}

static inline void lock(lock_t *lock)
{
    tidex_mutex_lock(&lock->mutex);

}

static inline void unlock(lock_t *lock)
{
    tidex_mutex_unlock(&lock->mutex);
}

#elif LOCK_METHOD==TICKET_LOCK
#define LOCK_PAD 256
#define MUTEX_NAME "Ticket Lock"
typedef struct lock_t {
    ticket_mutex_t mutex;
} lock_t;

static inline void lock_init(lock_t *lock)
{
    ticket_mutex_init(&lock->mutex);
}

static inline void lock(lock_t *lock)
{
    ticket_mutex_lock(&lock->mutex);

}

static inline void unlock(lock_t *lock)
{
    ticket_mutex_unlock(&lock->mutex);
}

#elif LOCK_METHOD==TIDEX_NPS_LOCK
#define LOCK_PAD 256
#define MUTEX_NAME "Tidex_Nps Lock"
typedef struct lock_t {
    tidex_nps_mutex_t mutex;
} lock_t;

static inline void lock_init(lock_t *lock)
{
    tidex_nps_mutex_init(&lock->mutex);
}

static inline void lock(lock_t *lock)
{
    tidex_nps_mutex_lock(&lock->mutex);

}

static inline void unlock(lock_t *lock)
{
    tidex_nps_mutex_unlock(&lock->mutex);
}

#endif

typedef struct thread_data_t {
    int          index;
    hwloc_obj_t  obj;
    int          nconsumers;
    int          nproducers;
    int          messages_per_thread;
    int          total_messages;
    int          randomize;
} thread_data_t;

typedef struct q_node_t {
    struct q_node_t *volatile  next;
} q_node_t;

typedef struct q_t {
    q_node_t *volatile  head;
    q_node_t           *tail;
    lock_t lock;
    char pad[LOCK_PAD-2*sizeof(q_node_t *)-sizeof(lock_t)];
} q_t;

void q_create(q_t *self)
{
    self->head = NULL;
    self->tail = NULL;
    lock_init(&self->lock);
}

void q_push(q_t *self, q_node_t *n)
{
    lock(&self->lock);
    n->next = NULL;

    if(self->head == NULL) {
        self->head = n;
        self->tail = n;
    } else {
        self->head->next = n;
        self->head = n;
    }

    unlock(&self->lock);
    return;
}

q_node_t *q_pop(q_t *self)
{
    q_node_t *rc;

    lock(&self->lock);

    if(self->tail == NULL) {
        unlock(&self->lock);
        return NULL;
    }

    rc = self->tail;

    if(!self->tail->next) {
        self->head = NULL;
        self->tail = NULL;
    } else self->tail = self->tail->next;

    unlock(&self->lock);
    return rc;
}

q_t *Q;
q_t *initQ(int nconsumers, int nproducers, int nmessages)
{
    q_t *arr = (q_t *)calloc(nconsumers,sizeof(q_t));
    return arr;
}


typedef struct work_node_t {
    q_node_t     node;
    int          id;
    int          data;
    char pad[64-sizeof(q_node_t) -
             sizeof(int)         -
             sizeof(int)];
} work_node_t;

void *do_produce(void *clientdata)
{
    int i;
    char *str;
    char *str1;
    thread_data_t *tdata  = (thread_data_t *)clientdata;
    int            me     = tdata->index;
    int            q      = me % tdata->nconsumers;
    hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
    hwloc_bitmap_zero(cpuset);
    hwloc_get_cpubind(g_topo, cpuset, HWLOC_CPUBIND_THREAD);
    hwloc_bitmap_asprintf(&str, cpuset);

    asprintf(&str1, "Producer %03d: token=XXX:", tdata->index);

    /* staged printf barrier */
    for(i=0; i<tdata->nconsumers+tdata->nproducers+PRINT_BATCH; i+=PRINT_BATCH) {
        pthread_barrier_wait(&g_barrier);
        int group0 = i/PRINT_BATCH;
        int group1 = me/PRINT_BATCH;

        if(group0 == group1 && PRINT_AFFINITY)
            printme(str1);

    }
    work_node_t *nodes_tmp = NULL;
    work_node_t *nodes     = (work_node_t *)malloc(sizeof(work_node_t) * tdata->messages_per_thread);
    int         *permute   = (int *)malloc(sizeof(int) *tdata->nconsumers);

    for(i=0; i<tdata->nconsumers; i++)
        permute[i]=i;

    for(i=0; i<tdata->nconsumers; i++) {
        int swapme, index;
        index=urandom(g_random_fd)%tdata->nconsumers;
        swapme = permute[i];
        permute[i] = permute[index];
        permute[index] = swapme;
    }

    for(i = tdata->messages_per_thread-1; i >= 0; --i) {
        nodes[i].id   = me;
        nodes[i].data = i+1;
    }

    /* Start Timer Barrier */
    pthread_barrier_wait(&g_barrier);
    pthread_barrier_wait(&g_barrier);

    /* End Timer Barrier */
    pthread_barrier_wait(&g_barrier);
    DEBUG_PRINT("Thread %d beginning, using q=%d\n", me,q);

    for(i = tdata->messages_per_thread-1; i >= 0; --i) {
        if(tdata->randomize) {
            q = (q+1) % tdata->nconsumers;
            q_push(&Q[permute[q]], &nodes[i].node);
        } else
            q_push(&Q[q], &nodes[i].node);
    }

    DEBUG_PRINT("Thread %d finished producing!\n", nodes[i].id);
    hwloc_bitmap_free(cpuset);
    free(str);

    pthread_mutex_lock(&g_mutex);
    g_done++;
    pthread_mutex_unlock(&g_mutex);

    /* Signal the consumers to stop */
    if(g_done == tdata->nproducers) {
        DEBUG_PRINT("Thread %d finished producing!\n", nodes[0].id);
        nodes_tmp = (work_node_t *)malloc(sizeof(work_node_t) *
                                          tdata->messages_per_thread);
        for(i=0; i<tdata->nconsumers; i++) {
            nodes_tmp[i].data = 0;
            q_push(&Q[i], &nodes_tmp[i].node);
        }
    }

    /* End of job barrier for timing */
    pthread_barrier_wait(&g_barrier);

    /* End of job barrier for printing */
    pthread_barrier_wait(&g_barrier);

    if(nodes_tmp)free(nodes_tmp);
    free(nodes);
    free(permute);

    pthread_exit(NULL);
    return NULL;
}

void *do_consume(void *clientdata)
{
    char *str;
    char *str1;
    struct timeval ti, tf;
    int            i,me;
    hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
    thread_data_t *tdata = (thread_data_t *)clientdata;
    double         calls=0.0, sum=0.0;
    me = tdata->index;

    hwloc_bitmap_zero(cpuset);
    hwloc_get_cpubind(g_topo, cpuset, HWLOC_CPUBIND_THREAD);
    hwloc_bitmap_asprintf(&str, cpuset);
    asprintf(&str1, "Consumer %03d: token=FFF:", tdata->index);

    /* staged printf barrier */
    for(i=0; i<tdata->nconsumers+tdata->nproducers+PRINT_BATCH; i+=PRINT_BATCH) {
        pthread_barrier_wait(&g_barrier);
        int group0 = i/PRINT_BATCH;
        int group1 = me/PRINT_BATCH;

        if(group0 == group1 && PRINT_AFFINITY)
            printme(str1);

    }

    /* Start timer Barrier */
    pthread_barrier_wait(&g_barrier);
    pthread_barrier_wait(&g_barrier);
    gettimeofday(&ti, NULL);

    /* End Timer Barrier */
    pthread_barrier_wait(&g_barrier);
    int done = 0;

    while(!done) {
        work_node_t *node = (work_node_t *)q_pop(&Q[me]);
        sum++;
        calls++;

        if(!node) {
            continue;
        }

        DEBUG_PRINT("Consumer:  (tid=%d node data = %d pl=%d\n",
                    node->id, node->data, done);

        if(node->data == 0) {
            DEBUG_PRINT("Got 0 from node! assuming finished!\n");
            done = 1;
        }
    }

    DEBUG_PRINT("Consumer finished!\n");
    gettimeofday(&tf, NULL);
    /* End of job barrier for timing */
    pthread_barrier_wait(&g_barrier);

    /* End of job barrier for printing */
    pthread_barrier_wait(&g_barrier);
    long usec = ((tf.tv_sec - ti.tv_sec)*1000000L+tf.tv_usec) - ti.tv_usec;
    double usecF       = (double) usec;
    double n_producers = (double) tdata->nproducers/tdata->nconsumers;
    double n_msgs      = (double) tdata->messages_per_thread*n_producers;
    printf("Consumer %03d:  n_msgs=%f in %f usec n_producers handled=%f:  mmsgs/s=%f mmsg/call=%f\n",
           tdata->index, n_msgs, usecF,n_producers, n_msgs/usecF, sum/calls);
    hwloc_bitmap_free(cpuset);
    free(str);

    pthread_exit(NULL);
    return NULL;
}


int main(int argc,char *argv[])
{
    struct timeval   ti, tf;
    pthread_attr_t   attr;
    cpu_set_t        cpus;
    int              i, j, n, d, depth;
    hwloc_obj_t obj;
    int c, nproducers = 0, nconsumers=0, nmessages=0, randomize=0;

    while((c = getopt(argc, argv, "rp:c:m:")) != -1)
        switch(c) {
            case 'r':
                randomize = 1;
                break;

            case 'p':
                nproducers = atoi(optarg);
                break;

            case 'c':
                nconsumers = atoi(optarg);
                break;

            case 'm':
                nmessages = atoi(optarg);
                break;

            case '?':
                if(optopt == 'p')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);

                if(optopt == 'c')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);

                if(optopt == 'm')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if(isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr,
                            "Unknown option character `\\x%x'.\n",
                            optopt);

                return 1;

            default:
                abort();
        }

    if(nproducers < 1 || nconsumers < 1 || (nproducers < nconsumers) ||
       nmessages < nconsumers) {
        fprintf(stderr, "Usage:  -p <num> -c <num> -m with (p <= c) and (m >= c)\n");
        return 1;
    }

    pthread_t        producers[nproducers];
    pthread_t        consumers[nconsumers];
    thread_data_t    producer_data[nproducers];
    thread_data_t    consumer_data[nconsumers];
    int              messages_per_thread = nmessages/nproducers;
    int              total_messages      = messages_per_thread*nproducers;
    Q = initQ(nconsumers, nproducers, nmessages);
    g_random_fd = urandom_init();
    printf("Starting %s queue with locks P:%d C:%d N:%d\n",
           MUTEX_NAME,nproducers, nconsumers,nmessages);
    hwloc_topology_init(&g_topo);
    hwloc_topology_load(g_topo);

    depth=hwloc_topology_get_depth(g_topo);

    for(d=0; d<depth; d++) {
        n=hwloc_get_nbobjs_by_depth(g_topo,d);

        for(i=0; i<n; i++) {
            obj=hwloc_get_obj_by_depth(g_topo,d,i);
#if 0
            printf("%d, %d type %d : %s\n",
                   d,i,obj->type, hwloc_obj_type_string(obj->type));
#endif
        }
    }

    n   = hwloc_get_nbobjs_by_type(g_topo, HWLOC_OBJ_CORE);

    /*hwloc_set_cpubind(t, obj->cpuset, 0);*/
    for(i=0; i<n; i++) {
        obj = hwloc_get_obj_by_type(g_topo, HWLOC_OBJ_CORE, i);
#if 0
        printf("%d, %d type %d : %s cpuset=0x%x\n",
               0,i,obj->type, hwloc_obj_type_string(obj->type),
               obj->cpuset);
#endif
        /*hwloc_cpuset_to_glibc_sched_affinity (hwloc_topology_t topology ,
          hwloc_const_cpuset_t hwlocset, cpu_set_t *schedset, size_t schedsetsize) */
        /*hwloc_cpuset_to_glibc_sched_affinity(topology,
          obj->cpuset, &mask, sizeof(mask));*/
    }

    pthread_attr_init(&attr);
    pthread_barrier_init(&g_barrier, NULL, nproducers+nconsumers+1);

    for(i=0; i < nconsumers; i++) {
        q_create(&Q[i]);
    }

    for(i=0,j=1; i < nproducers; i++) {
        CPU_ZERO(&cpus);
        obj = hwloc_get_obj_by_type(g_topo, HWLOC_OBJ_CORE, j);
        hwloc_cpuset_to_glibc_sched_affinity(g_topo,obj->cpuset,
                                             &cpus,sizeof(cpus));
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        producer_data[i].index= i;
        producer_data[i].obj  = obj;
        producer_data[i].nconsumers          = nconsumers;
        producer_data[i].nproducers          = nproducers;
        producer_data[i].messages_per_thread = messages_per_thread;
        producer_data[i].total_messages      = total_messages;
        producer_data[i].randomize           = randomize;

        int ret = pthread_create(producers + i, &attr, do_produce, (void *)&producer_data[i]);

        if(ret != 0) {
            exit(1);
        } else {
            DEBUG_PRINT("Spawned consumer thread %d\n", i);
        }

        if(i<=(nconsumers-2)) j+=2;
        else j+=1;
    }

    for(i=0, j=0; i < nconsumers; i++,j+=2) {
        CPU_ZERO(&cpus);
        obj = hwloc_get_obj_by_type(g_topo, HWLOC_OBJ_CORE, j);
        hwloc_cpuset_to_glibc_sched_affinity(g_topo,obj->cpuset,
                                             &cpus,sizeof(cpus));
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);

        consumer_data[i].index= i;
        consumer_data[i].obj  = obj;
        consumer_data[i].nconsumers          = nconsumers;
        consumer_data[i].nproducers          = nproducers;
        consumer_data[i].messages_per_thread = messages_per_thread;
        consumer_data[i].total_messages      = total_messages;
        consumer_data[i].randomize           = randomize;

        int ret = pthread_create(consumers+i, &attr, do_consume, (void *)&consumer_data[i]);

        if(ret != 0) {
            exit(1);
        } else {
            DEBUG_PRINT("Spawned consumer thread %d\n",i);
        }
    }

    /* staged printf Barrier */
    for(i=0; i<nconsumers+nproducers+PRINT_BATCH; i+=PRINT_BATCH)
        pthread_barrier_wait(&g_barrier);

    /* Start timer Barrier */
    pthread_barrier_wait(&g_barrier);
    pthread_barrier_wait(&g_barrier);
    gettimeofday(&ti, NULL);

    /* End timer Barrier */
    pthread_barrier_wait(&g_barrier);

    /* End of job barrier for timing */
    pthread_barrier_wait(&g_barrier);
    gettimeofday(&tf, NULL);

    /* End of job barrier for printing */
    pthread_barrier_wait(&g_barrier);

    for(i=0; i < nproducers; i++) {
        pthread_join(producers[i], NULL);
    }

    for(i=0; i < nconsumers; i++) {
        pthread_join(consumers[i], NULL);
    }

    long usec = ((tf.tv_sec - ti.tv_sec)*1000000L+tf.tv_usec) - ti.tv_usec;
    double usecF       = (double) usec;
    double n_msgs      = (double) total_messages;
    double n_producers = (double) nproducers;
    printf("Time in microseconds: %f\n",usecF);
    printf("n_msgs=%f n_producers=%f:  mmsgs/s=%f  mmsgs/s/producer=%f\n",
           n_msgs, n_producers, n_msgs/usecF,
           n_msgs/usecF/n_producers);
    printf("DATAOUT %d %d %d %f %f\n",
           nproducers,nconsumers,total_messages,
           n_msgs/usecF,n_msgs/usecF/n_producers);
    return 0;
}
