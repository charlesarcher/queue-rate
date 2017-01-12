/*
 * ------------------------------------------------------------------------
 * Lock-free N-producers M-consumers ring-buffer queue.
 * ABA problem safe.
 *
 * This implementation is bit complicated, so possibly it has sense to use
 * classic list-based queues. See:
 * 1. D.Fober, Y.Orlarey, S.Letz, "Lock-Free Techniques for Concurrent
 *    Access to Shared Ojects"
 * 2. M.M.Michael, M.L.Scott, "Simple, Fast and Practical Non-Blocking and
 *    Blocking Concurrent Queue Algorithms"
 * 3. E.Ladan-Mozes, N.Shavit, "An Optimistic Approach to Lock-Free FIFO Queues"
 *
 * See also implementation of N-producers M-consumers FIFO and
 * 1-producer 1-consumer ring-buffer from Tim Blechmann:
 *	http://tim.klingt.org/boost_lockfree/
 *	git://tim.klingt.org/boost_lockfree.git
 * 
 * See See Intel 64 and IA-32 Architectures Software Developer's Manual,
 * Volume 3, Chapter 8.2 Memory Ordering for x86 memory ordering guarantees.
 * ------------------------------------------------------------------------
 */
#define QUEUE_SIZE 32*1024
#include <malloc.h>
#include <immintrin.h>
#include <algorithm>

static size_t __thread __thr_id;

/**
 * @return continous thread IDs starting from 0 as opposed to pthread_self().
 */
inline size_t
thr_id()
{
	return __thr_id;
}

inline void
set_thr_id(size_t id)
{
	__thr_id = id;
}

template<class T,
	decltype(thr_id) ThrId = thr_id,
	unsigned long Q_SIZE = QUEUE_SIZE>
class LockFreeQueue {
private:
	static const unsigned long Q_MASK = Q_SIZE - 1;

	struct ThrPos {
		unsigned long head, tail;
	};

public:
	LockFreeQueue(size_t n_producers, size_t n_consumers)
		: n_producers_(n_producers),
		n_consumers_(n_consumers),
		head_(0),
		tail_(0),
		last_head_(0),
		last_tail_(0)
	{
		auto n = std::max(n_consumers_, n_producers_);
		thr_p_ = (ThrPos *)::memalign(getpagesize(), sizeof(ThrPos) * n);
		assert(thr_p_);
		// Set per thread tail and head to ULONG_MAX.
		::memset((void *)thr_p_, 0xFF, sizeof(ThrPos) * n);

		ptr_array_ = (T **)::memalign(getpagesize(),
				Q_SIZE * sizeof(void *));
		assert(ptr_array_);
	}

	~LockFreeQueue()
	{
		::free(ptr_array_);
		::free(thr_p_);
	}

	ThrPos&
	thr_pos() const
	{
		assert(ThrId() < std::max(n_consumers_, n_producers_));
		return thr_p_[ThrId()];
	}

	void
	push(T *ptr)
	{
		/*
		 * Request next place to push.
		 *
		 * Second assignemnt is atomic only for head shift, so there is
		 * a time window in which thr_p_[tid].head = ULONG_MAX, and
		 * head could be shifted significantly by other threads,
		 * so pop() will set last_head_ to head.
		 * After that thr_p_[tid].head is setted to old head value
		 * (which is stored in local CPU register) and written by @ptr.
		 *
		 * First assignment guaranties that pop() sees values for
		 * head and thr_p_[tid].head not greater that they will be
		 * after the second assignment with head shift.
		 *
		 * Loads and stores are not reordered with locked instructions,
		 * se we don't need a memory barrier here.
		 */
		thr_pos().head = head_;
		thr_pos().head = __sync_fetch_and_add(&head_, 1);

		/*
		 * We do not know when a consumer uses the pop()'ed pointer,
		 * se we can not overwrite it and have to wait the lowest tail.
		 */
		while (__builtin_expect(thr_pos().head >= last_tail_ + Q_SIZE, 0))
		{
			auto min = tail_;

			// Update the last_tail_.
			for (size_t i = 0; i < n_consumers_; ++i) {
				auto tmp_t = thr_p_[i].tail;

				// Force compiler to use tmp_h exactly once.
				asm volatile("" ::: "memory");

				if (tmp_t < min)
					min = tmp_t;
			}
			last_tail_ = min;

			if (thr_pos().head < last_tail_ + Q_SIZE)
				break;
			_mm_pause();
		}

		ptr_array_[thr_pos().head & Q_MASK] = ptr;

		// Allow consumers eat the item.
		thr_pos().head = ULONG_MAX;
	}

	T *
	pop()
	{
		/*
		 * Request next place from which to pop.
		 * See comments for push().
		 *
		 * Loads and stores are not reordered with locked instructions,
		 * se we don't need a memory barrier here.
		 */
		thr_pos().tail = tail_;
		thr_pos().tail = __sync_fetch_and_add(&tail_, 1);

		/*
		 * tid'th place in ptr_array_ is reserved by the thread -
		 * this place shall never be rewritten by push() and
		 * last_tail_ at push() is a guarantee.
		 * last_head_ guaraties that no any consumer eats the item
		 * before producer reserved the position writes to it.
		 */
		while (__builtin_expect(thr_pos().tail >= last_head_, 0))
		{
			auto min = head_;

			// Update the last_head_.
			for (size_t i = 0; i < n_producers_; ++i) {
				auto tmp_h = thr_p_[i].head;

				// Force compiler to use tmp_h exactly once.
				asm volatile("" ::: "memory");

				if (tmp_h < min)
					min = tmp_h;
			}
			last_head_ = min;

			if (thr_pos().tail < last_head_)
				break;
			_mm_pause();
		}

		T *ret = ptr_array_[thr_pos().tail & Q_MASK];
		// Allow producers rewrite the slot.
		thr_pos().tail = ULONG_MAX;
		return ret;
	}

private:
	/*
	 * The most hot members are cacheline aligned to avoid
	 * False Sharing.
	 */
#define ____cacheline_aligned	__attribute__((aligned(64)))
	const size_t n_producers_, n_consumers_;
	// currently free position (next to insert)
	unsigned long	head_ ____cacheline_aligned;
	// current tail, next to pop
	unsigned long	tail_ ____cacheline_aligned;
	// last not-processed producer's pointer
	unsigned long	last_head_ ____cacheline_aligned;
	// last not-processed consumer's pointer
	unsigned long	last_tail_ ____cacheline_aligned;
	ThrPos		*thr_p_;
	T		**ptr_array_;
};
