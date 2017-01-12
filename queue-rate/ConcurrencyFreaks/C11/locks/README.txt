This folder contains multiple implementations of Mutual Exclusion Locks in C11.
They are cross-platform.


-------------------------------------------------------------------------------
Ticket Lock      

ticket_mutex.h 
ticket_mutex.c
A simple Ticket Lock based on the algorithm described here:
http://web.mit.edu/6.173/www/currentsemester/readings/R06-scalable-synchronization-1991.pdf
with the optimizations described in this post:
http://concurrencyfreaks.com/2014/05/relaxed-atomics-optimizations-for.html



-------------------------------------------------------------------------------
Multi-Producer-Single-Consumer Mutex

mpsc_mutex.h
mpsc_mutex.c
A mutual exclusion lock that uses the MPSC queue invented by Dmitry Vyukov.
This is actually a simple variant of the CLH lock (see below).
More details can be seen here:
http://concurrencyfreaks.com/2014/05/c11-atomics-and-mpsc-mutual-exclusion.html
and a powerpoint presentation here:
https://github.com/pramalhe/ConcurrencyFreaks/tree/master/presentations/MPSC-Mutex.pptx



-------------------------------------------------------------------------------
CLH Lock

clh_mutex.h
clh_mutex.c
A mutual exclusion lock discovered independently by Travis Craig at the 
University of Washington (UW TR 93-02-02, February 1993), and by 
Anders Landin and Eric Hagersten of the Swedish Institute of Computer Science (IPPS, 1994).
http://www.cs.rochester.edu/research/synchronization/pseudocode/ss.html#clh
More details can be seen here
http://concurrencyfreaks.com/2014/05/exchg-mutex-alternative-to-mcs-lock.html
and a powerpoint presentation here:
https://github.com/pramalhe/ConcurrencyFreaks/tree/master/presentations/CLH-Mutex.pptx



-------------------------------------------------------------------------------
CLH Reader-Writer Lock

clh_rwlock.h
clh_rwlock.c
A Reader-Writer lock that uses the same kind of ideas as the CLH mutex.
More details can be seen here:
http://concurrencyfreaks.com/2014/06/clh-reader-writer-lock.html
and a powerpoint presentation here:
https://github.com/pramalhe/ConcurrencyFreaks/tree/master/presentations/CLH-RWLock.pptx



-------------------------------------------------------------------------------
Tidex Mutex

tidex_mutex.h
tidex_mutex.c
A new algorithm for a mutual exclusion lock that is a kind of 
oversimplification of the CLH lock but has properties similar to a Ticket Lock
and uses an atomic_exchange() instead of an atomic_fetch_add().
It is starvation-free.
More details can be seen here:
http://concurrencyfreaks.com/2014/12/tidex-mutex.html



-------------------------------------------------------------------------------
Tidex Mutex - No Pthread-Self variant

tidex_nps_mutex.h
tidex_nps_mutex.c
Same as Tidex Mutex but without using pthread_self().
More details here:
http://concurrencyfreaks.com/2014/12/tidex-mutex-no-pthreadself-variant.html



-------------------------------------------------------------------------------
Ticket AWN - Negative Egress variant

ticketawn/ticket_awnne_mutex.h
ticketawn/ticket_awnne_mutex.c
Ticket Lock with Array of Waiting Nodes (AWN) - Negative Egress variant
In the "Negative Egress" variant we set the egress to negative to indicate that
the thread calling unlock() has seen the waiting node from the next waiting
thread.
More details here:
http://concurrencyfreaks.com/2015/01/ticket-lock-array-of-waiting-nodes-awn.html



-------------------------------------------------------------------------------
Ticket AWN - Ends on Egress variant

ticketawn/ticket_awnee_mutex.h
ticketawn/ticket_awnee_mutex.c
Ticket Lock with Array of Waiting Nodes (AWN) - Ends on Egress variant
In the "Ends on Egress" variant, we always scan the egress one last time before
acquiring the lock.
More details here:
http://concurrencyfreaks.com/2015/01/ticket-lock-array-of-waiting-nodes-awn.html



-------------------------------------------------------------------------------
Ticket AWN - Spins on Both variant

ticketawn/ticket_awnsb_mutex.h
ticketawn/ticket_awnsb_mutex.c
Ticket Lock with Array of Waiting Nodes (AWN) - Spins on Both variant
In the "Spins on Both" variant, the thread acquiring the lock may have to spin
on both lockIsMine and egress, waiting for either one of them to allow the lock
to be acquired.
More details here:
http://concurrencyfreaks.com/2015/01/ticket-lock-array-of-waiting-nodes-awn.html



