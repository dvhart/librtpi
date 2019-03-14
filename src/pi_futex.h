#ifndef PI_FUTEX_H
#define PI_FUTEX_H

#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>

static inline __u32 get_op(__u32 op, __u32 mod)
{
	if (!(mod & RTPI_MUTEX_PSHARED))
		op |= FUTEX_PRIVATE_FLAG;
	return op;
}

/**
 * sys_futex() - Futex syscall wrapper
 * @uaddr:	address of first futex
 * @op:		futex op code
 * @val:	typically expected value of uaddr, but varies by op
 * @utime:	typically an absolute struct timespec, overloaded by some ops
 * @uaddr2:	address of second futex for some ops
 * @val3:	varies by op
 */
static inline int sys_futex(__u32 *uaddr, int op, __u32 val,
			    const struct timespec *restrict utime,
			    __u32 *uaddr2, __u32 val3)
{
	return syscall(SYS_futex, uaddr, op, val, utime, uaddr2, val3);
}

/**
 * futex_lock_pi() - block on a PI mutex
 * @mutex: PI mutex to block on
 */
static inline int futex_lock_pi(pi_mutex_t *mutex)
{
	return sys_futex(&mutex->futex,
			 get_op(FUTEX_LOCK_PI, mutex->flags),
			 0,    /* deadlock detection (no) */
			 NULL, /* timeout (none) */
			 NULL, /* uaddr2 unused */
			 0);   /* val3 unused */
}

/**
 * futex_unlock_pi() - release PI mutex, wake the top waiter
 * @mutex: PI mutex to release
 */
static inline int futex_unlock_pi(pi_mutex_t *mutex)
{
	return sys_futex(&mutex->futex,
			 get_op(FUTEX_UNLOCK_PI, mutex->flags),
			 0,    /* deadlock detection unused */
			 NULL, /* timeout unused */
			 NULL, /* uaddr2 unused */
			 0);   /* val3 unused */
}

/**
 * futex_wait_requeue_pi() - wait on a condition variable, setup for requeue PI
 * @cond: condition variable to wait on (containing non-PI futex)
 * @val: expected value of condition variable futex
 * @utime: absolute timeout
 * @mutex: PI mutex containing PI futex target
 */
static inline int futex_wait_requeue_pi(pi_cond_t *cond, __u32 val,
					const struct timespec *restrict utime,
					pi_mutex_t *mutex)
{
	return sys_futex(&cond->cond,
			 get_op(FUTEX_WAIT_REQUEUE_PI, cond->flags),
			 val,
			 utime,
			 &mutex->futex,
			 0);            /* val3 unused */
}

/**
 * futex_cmp_requeue_pi() - requeue from condition variable to PI mutex
 * @cond: condition variable to requeue from (containing non-PI futex)
 * @val: expected value of cond futex (ignored, assumed to be 1, forcing syscall)
 * @nr_requeue: number of waiters to requeue
 * @mutex: PI mutex to requeue to (containing PI futex)
 */
static inline int futex_cmp_requeue_pi(pi_cond_t *cond, __u32 val,
				       __u32 nr_requeue, pi_mutex_t *mutex)
{
	return sys_futex(&cond->cond,
			 get_op(FUTEX_CMP_REQUEUE_PI, cond->flags),
			 1,                        /* nr_wake */
			 (void *)(long)nr_requeue,
			 &mutex->futex,
			 val);
}

#endif
