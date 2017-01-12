// -*- mode: c++; c-basic-offset:4 ; indent-tabs-mode:nil ; -*-
#ifndef __TBB_Q_H__
#define __TBB_Q_H__

#include "concurrentqueue/benchmarks/tbb/concurrent_queue.h" /* TBB queue */
#define QUEUE_NAME "Tbb Queue"
typedef tbb::concurrent_queue<work_node_t *> Q_t;
class token_t { public: token_t(Q_t &q) {} };
typedef token_t producer_token_t;
typedef token_t consumer_token_t;
Q_t *Q;
Q_t *initQ(int nconsumers, int nproducers, int nmessages)
{
    Q_t *arr = static_cast<Q_t *>(::operator new[](nconsumers*sizeof(Q_t)));

    for(int i = 0; i < nconsumers; i++) {
        ::new(arr+i) Q_t();
    }

    return arr;
}

static inline void enqueue_tok(Q_t                    &inQ,
                               const producer_token_t &token,
                               work_node_t            &work)
{
    inQ.push(&work);
}

static inline void enqueue(Q_t                    &inQ,
                           work_node_t            &work)
{
    inQ.push(&work);
}

static inline int try_dequeue_bulk(Q_t                     &inQ,
                                   work_node_t            *&head,
                                   int                     num)
{
    bool good;
    good = inQ.try_pop(head);

    if(good) return 1;
    else return 0;
}

static inline int try_dequeue_bulk_tok(Q_t               &inQ,
                                       consumer_token_t  &tok,
                                       work_node_t      *&head,
                                       int                num)
{
    bool good;
    good = inQ.try_pop(head);

    if(good) return 1;
    else return 0;
}

#endif /* __TBB_Q_H__ */
