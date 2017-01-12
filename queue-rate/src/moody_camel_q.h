// -*- mode: c++; c-basic-offset:4 ; indent-tabs-mode:nil ; -*-
#ifndef __MOODY_CAMEL_QUEUE_H__
#define __MOODY_CAMEL_QUEUE_H__

#include "concurrentqueue/concurrentqueue.h"                 /* Moody Camel Queue    */
#define QUEUE_NAME "Moody Camel Queue"
typedef moodycamel::ConcurrentQueue<work_node_t *>                   Q_t;
typedef moodycamel::ConcurrentQueue<work_node_t *>::producer_token_t producer_token_t;
typedef moodycamel::ConcurrentQueue<work_node_t *>::consumer_token_t consumer_token_t;
Q_t *Q;
Q_t *initQ(int nconsumers, int nproducers, int nmessages)
{
    Q_t *arr = static_cast<Q_t *>(::operator new[](nconsumers*sizeof(Q_t)));

    for(int i = 0; i < nconsumers; i++) {
        ::new(arr+i) Q_t(nmessages);
    }

    return arr;
}
static inline void enqueue_tok(Q_t                    &inQ,
                               const producer_token_t &token,
                               work_node_t            &work)
{
    inQ.enqueue(token, &work);
}

static inline void enqueue(Q_t                    &inQ,
                           work_node_t            &work)
{
    inQ.enqueue(&work);
}

static inline int try_dequeue_bulk(Q_t                     &inQ,
                                   work_node_t            *&head,
                                   int                     num)
{
    return inQ.try_dequeue_bulk(&head,num);
}

static inline int try_dequeue_bulk_tok(Q_t               &inQ,
                                       consumer_token_t  &tok,
                                       work_node_t      *&head,
                                       int                num)
{
    return inQ.try_dequeue_bulk(tok,&head,num);
}

#endif /* __MOODY_CAMEL_QUEUE_H__ */
