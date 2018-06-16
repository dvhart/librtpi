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

static inline int sys_futex(__u32 *addr, int op, __u32 val,
			    const struct timespec *restrict utime,
			    __u32 *uaddr2, __u32 val3)
{
	return syscall(SYS_futex, addr, op, val, utime, uaddr2, val3);
}

static inline int futex_lock_pi(pi_mutex_t *mutex)
{
	return sys_futex(&mutex->futex,
			 get_op(FUTEX_LOCK_PI, mutex->flags),
			 0, NULL, 0, 0);
}

static inline int futex_unlock_pi(pi_mutex_t *mutex)
{
	return sys_futex(&mutex->futex,
			 get_op(FUTEX_UNLOCK_PI, mutex->flags),
			 0, NULL, 0, 0);
}

static inline int futex_wait_requeue_pi(pi_cond_t *cond, __u32 val,
					const struct timespec *restrict utime,
					pi_mutex_t *mutex, __u32 val3)
{
	return sys_futex(&cond->cond,
			 get_op(FUTEX_WAIT_REQUEUE_PI, cond->flags),
			 val, utime,
			 &mutex->futex, val3);
}

static inline int futex_cmp_requeue_PI(pi_cond_t *cond, __u32 val,
				       __u32 val2,
				       pi_mutex_t *mutex, __u32 val3)
{
	return sys_futex(&cond->cond,
			 get_op(FUTEX_CMP_REQUEUE_PI, cond->flags),
			 1,
			 (void *)(long)val2, &mutex->futex,
			 val3);
}

#endif
