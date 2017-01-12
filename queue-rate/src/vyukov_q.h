// -*- mode: c++; c-basic-offset:4 ; indent-tabs-mode:nil ; -*-
#ifndef __VYUKOV_Q_H__
#define __VYUKOV_Q_H__

#define QUEUE_NAME "Vyukov Queue"

typedef union mpsc_node_t {
    mpsc_node_t   *volatile next;
    work_node_t  wn;
} mpsc_node_t;

struct mpscq_t {
    mpsc_node_t *volatile  head;
    mpsc_node_t           *tail;
    mpsc_node_t            stub;
};

void mpscq_create(mpscq_t *self)
{
    self->head = &self->stub;
    self->tail = &self->stub;
    self->stub.next = 0;

}

void mpscq_push(mpscq_t *self, work_node_t *in_n)
{
    mpsc_node_t *n = (mpsc_node_t *)in_n;
    n->next = 0;
    mpsc_node_t *prev = __sync_lock_test_and_set(&self->head, n);
    prev->next = n;
}

work_node_t *mpscq_pop(mpscq_t *self)
{
    mpsc_node_t *tail = self->tail;
    mpsc_node_t *next = tail->next;

    if(tail == &self->stub) {
        if(0 == next)
            return 0;

        self->tail = next;
        tail = next;
        next = next->next;
    }

    if(next) {
        self->tail = next;
        return &tail->wn;
    }

    mpsc_node_t *head = self->head;

    if(tail != head)
        return 0;

    mpscq_push(self, &self->stub.wn);
    next = tail->next;

    if(next) {
        self->tail = next;
        return &tail->wn;
    }

    return 0;
}

typedef struct mpscq_t Q_t;
class token_t { public: token_t(Q_t &q) {} };
typedef token_t producer_token_t;
typedef token_t consumer_token_t;
Q_t *Q;
Q_t *initQ(int nconsumers, int nproducers, int nmessages)
{
    Q_t *arr = static_cast<Q_t *>(::operator new[](nconsumers*sizeof(Q_t)));

    for(int i = 0; i < nconsumers; i++) {
        ::new(arr+i) Q_t();
        mpscq_create(arr+i);
    }

    return arr;
}

static inline void enqueue_tok(Q_t                    &inQ,
                               const producer_token_t &token,
                               work_node_t            &work)
{
    mpscq_push(&inQ,&work);
}

static inline void enqueue(Q_t                    &inQ,
                           work_node_t            &work)
{
    mpscq_push(&inQ,&work);
}

static inline int try_dequeue_bulk(Q_t                     &inQ,
                                   work_node_t            *&head,
                                   int                     num)
{
    head = mpscq_pop(&inQ);

    if(head) return 1;
    else return 0;
}

static inline int try_dequeue_bulk_tok(Q_t               &inQ,
                                       consumer_token_t  &tok,
                                       work_node_t      *&head,
                                       int                num)
{
    head = mpscq_pop(&inQ);

    if(head) return 1;
    else return 0;
}

#endif /* __VYUKOV_Q_H__ */
