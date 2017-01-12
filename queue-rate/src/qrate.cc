// -*- mode: c++; c-basic-offset:4 ; indent-tabs-mode:nil ; -*-
#include <hwloc.h>
#include <hwloc/glibc-sched.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <new>
/* -------------------------------------------------------------------  */
/* |Facebook Folly  | https://github.com/facebook/folly               | */
/* |Moody Camel     | https://github.com/cameron314/concurrentqueue   | */
/* |Cloudius        | https://github.com/cloudius-systems/osv         | */
/* |Natsys Queue    | https://github.com/natsys/blog                  | */
/* -------------------------------------------------------------------  */
#define FOLLY_QUEUE        1
#define MOODY_CAMEL_QUEUE  2
#define CLOUDIUS_QUEUE     3
#define NATSYS_QUEUE       4
#define VYUKOV_QUEUE       5
#define TBB_QUEUE          6
#define BULK_DEQUEUE       524288

//#define DEBUG
extern "C" {
extern int printme(char *instr);
extern int urandom_init();
extern unsigned long urandom(int urandom_fd);
}

#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#else
#define DEBUG_PRINT(...) do{ } while ( 0 )
#endif

#define PRINT_BATCH 2
#define PRINT_AFFINITY 0

typedef struct thread_data_t {
    int          index;
    hwloc_obj_t  obj;
    int          nconsumers;
    int          nproducers;
    int          messages_per_thread;
    int          total_messages;
    int          randomize;
} thread_data_t;
typedef struct work_node_t {
    work_node_t *next;
    int          id;
    int          data;
    char pad[64-sizeof(work_node_t *) -
             sizeof(int)              -
             sizeof(int)];
} work_node_t;

pthread_mutex_t   g_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t g_barrier;
hwloc_topology_t  g_topo;
int               g_done;
int               g_random_fd;


#if  QUEUE_METHOD==FOLLY_QUEUE
#include "folly_q.h"
#elif QUEUE_METHOD==MOODY_CAMEL_QUEUE
#include "moody_camel_q.h"
#elif QUEUE_METHOD==CLOUDIUS_QUEUE
#include "cloudius_q.h"
#elif QUEUE_METHOD==NATSYS_QUEUE
#include "natsys_q.h"
#elif QUEUE_METHOD==VYUKOV_QUEUE
#include "vyukov_q.h"
#elif QUEUE_METHOD==TBB_QUEUE
#include "tbb_q.h"
#elif QUEUE_METHOD==BOOST_QUEUE
#include "boost_q.h"
#else
#error "A valid queue method has not been chosen"
#endif

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

    asprintf(&str1, "Producer %03d: token=%03d:", tdata->index,q);

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
        nodes[i].data = 1+i;
    }

    producer_token_t prodTok(Q[q]);
    /* Start Timer Barrier */
    pthread_barrier_wait(&g_barrier);
    pthread_barrier_wait(&g_barrier);

    /* End Timer Barrier */
    pthread_barrier_wait(&g_barrier);
    DEBUG_PRINT("Thread %d beginning, using q=%d\n",me,q);

    for(i = tdata->messages_per_thread-1; i >= 0; --i) {
        if(tdata->randomize) {
            q = (q+1) % tdata->nconsumers;
            enqueue(Q[permute[q]],nodes[i]);
        } else
            enqueue_tok(Q[q],prodTok, nodes[i]);
    }

    DEBUG_PRINT("Thread %d finished producing!\n", nodes[0].id);
    hwloc_bitmap_free(cpuset);
    free(str);

    pthread_mutex_lock(&g_mutex);
    g_done++;
    pthread_mutex_unlock(&g_mutex);

    /* Signal the consumers to stop                        */
    /* Use temporary nodes...yours might still be enqueued */
    if(g_done == tdata->nproducers) {
        DEBUG_PRINT("Thread %d finished producing!\n", nodes[0].id);
        nodes_tmp = (work_node_t *)malloc(sizeof(work_node_t) *
                                          tdata->messages_per_thread);
        for(i=0; i<tdata->nconsumers; i++) {
            nodes_tmp[i].data = 0;
            enqueue(Q[i],nodes_tmp[i]);
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
    consumer_token_t consTok(Q[me]);

    hwloc_bitmap_zero(cpuset);
    hwloc_get_cpubind(g_topo, cpuset, HWLOC_CPUBIND_THREAD);
    hwloc_bitmap_asprintf(&str, cpuset);

    if(tdata->nproducers==tdata->nconsumers)
        asprintf(&str1, "Consumer %03d: token=%03d:", tdata->index, me);
    else
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
    int done=0;

    while(!done) {
        work_node_t *node[BULK_DEQUEUE];
        unsigned i;
        size_t result;

        if(tdata->nproducers==tdata->nconsumers)
            result = try_dequeue_bulk_tok(Q[me],consTok,node[0],BULK_DEQUEUE);
        else
            result = try_dequeue_bulk(Q[me], node[0],BULK_DEQUEUE);

        sum+=result;
        calls++;

        if(result==0) {
            continue;
        }

        for(i=0; i< result; i++) {
            DEBUG_PRINT("Consumer:  (tid=%d node data = %d\n",
                        node[i]->id, node[i]->data);

            if(node[i]->data == 0) {
                DEBUG_PRINT("Got 0 from node! assuming finished!\n");
                done=1;
            }
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
    printf("Consumer %03d:  n_msgs=%f in %f usec n_producers handled=%f:  mmsgs/s=%f nmsg/call=%f\n",
           tdata->index, n_msgs, usecF,n_producers, n_msgs/usecF, sum/calls);
    hwloc_bitmap_free(cpuset);
    free(str);

    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char *argv[])
{
    struct timeval   ti, tf;
    pthread_attr_t   attr;
    cpu_set_t        cpus;
    int              i, j, n, d, depth;
    hwloc_obj_t      obj;
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

    printf("Starting mpsc with %s queue P:%d C:%d NM:%d\n",
           QUEUE_NAME,nproducers, nconsumers,nmessages);
    pthread_t        producers[nproducers];
    pthread_t        consumers[nconsumers];
    thread_data_t    producer_data[nproducers];
    thread_data_t    consumer_data[nconsumers];
    int              messages_per_thread = nmessages/nproducers;
    int              total_messages      = messages_per_thread*nproducers;
    Q = initQ(nconsumers, nproducers, nmessages);
    g_random_fd = urandom_init();
    hwloc_topology_init(&g_topo);
    hwloc_topology_load(g_topo);

    depth=hwloc_topology_get_depth(g_topo);

    for(d=0; d<depth; d++) {
        n=hwloc_get_nbobjs_by_depth(g_topo,d);

        for(i=0; i<n; i++) {
            obj=hwloc_get_obj_by_depth(g_topo,d,i);
        }
    }

    n   = hwloc_get_nbobjs_by_type(g_topo, HWLOC_OBJ_CORE);

    for(i=0; i<n; i++) {
        obj = hwloc_get_obj_by_type(g_topo, HWLOC_OBJ_CORE, i);
    }

    pthread_attr_init(&attr);
    pthread_barrier_init(&g_barrier, NULL, nproducers+nconsumers+1);

    for(i=0,j=1; i < nproducers; i++) {
        CPU_ZERO(&cpus);
        obj = hwloc_get_obj_by_type(g_topo, HWLOC_OBJ_CORE, j);
        hwloc_cpuset_to_glibc_sched_affinity(g_topo,obj->cpuset,
                                             &cpus,sizeof(cpus));
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        producer_data[i].index               = i;
        producer_data[i].obj                 = obj;
        producer_data[i].nconsumers          = nconsumers;
        producer_data[i].nproducers          = nproducers;
        producer_data[i].messages_per_thread = messages_per_thread;
        producer_data[i].total_messages      = total_messages;
        producer_data[i].randomize           = randomize;

        int ret = pthread_create(producers + i, &attr, do_produce, (void *)&producer_data[i]);

        if(ret != 0) {
            exit(1);
        } else {
            DEBUG_PRINT("Spawned producer thread %d\n", i);
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

        consumer_data[i].index               = i;
        consumer_data[i].obj                 = obj;
        consumer_data[i].nconsumers          = nconsumers;
        consumer_data[i].nproducers          = nproducers;
        consumer_data[i].messages_per_thread = messages_per_thread;
        consumer_data[i].total_messages      = total_messages;
        consumer_data[i].randomize           = randomize;

        int ret = pthread_create(consumers+i, &attr, do_consume, (void *)&consumer_data[i]);

        if(ret != 0) {
            exit(1);
        } else {
            DEBUG_PRINT("Spawned consumer thread %d\n", i);
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
