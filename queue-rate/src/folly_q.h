// -*- mode: c++; c-basic-offset:4 ; indent-tabs-mode:nil ; -*-
#ifndef __FOLLY_Q_H__
#define __FOLLY_Q_H__

#include "folly/folly/MPMCQueue.h"                           /* Facebook Folly Queue */
#define QUEUE_NAME "Facebook Folly Queue"
typedef folly::MPMCQueue<work_node_t *> Q_t;
class token_t { public: token_t(Q_t &q) {} };
typedef token_t producer_token_t;
typedef token_t consumer_token_t;
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
    inQ.write(&work);
}

static inline void enqueue(Q_t                    &inQ,
                           work_node_t            &work)
{
    inQ.write(&work);
}

static inline int try_dequeue_bulk(Q_t                     &inQ,
                                   work_node_t            *&head,
                                   int                     num)
{
    bool flag;
    flag = inQ.read(head);

    if(flag) {
        return 1;
    } else return 0;
}

static inline int try_dequeue_bulk_tok(Q_t               &inQ,
                                       consumer_token_t  &tok,
                                       work_node_t      *&head,
                                       int                num)
{
    bool flag;
    flag = inQ.read(head);

    if(flag) {
        return 1;
    } else return 0;
}

#endif /* __FOLLY_Q_H__ */
