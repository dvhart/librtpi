# Introduction
The Real-Time Priority Inheritance Library (librtpi) is intended to bridge the
gap between the glibc pthread implementation and a functionally correct priority
inheritance for pthread locking primitives, such as pthread_mutex and
pthread_condvar.

Specifically, priority based wakeup is required for correct operation, in
contrast to the more time-based ordered wakeup groups in the glibc pthread
condvar implementation.

A C library with C++ bindings are provided.

# Approach
This library provides an API that is as close as possible to the POSIX pthread
specification. Any changes or restrictions to this API are the result of
ensuring priority based wakeup ordering and proper behavior in the context of
real-time and priority inheritance requirements.

Wherever possible, the glibc pthread primitives and functions are used, and
where necessary are wrapped or rewritten. When the glibc implementation is
incompatible, a new mechanism is provided, e.g. the pthread_condvar
specifically.

To encourage programming to the API, internal types specific to the
implementation are opaque types, or private class members in the case of the C++
bindings (using the "Has a" vs. "Is a" inheritance model).

# Build
	$ autoreconf --install
	$ ./configure
	$ make
	$ ./test

# License and Copyright
The Real-Time Priority Inheritance Library is licensed under the Lesser GNU
Public License. The LGPL was chosen to make it possible to link with libc
libraries, reuse code from libc, and still be as broadly usable as possible in
industry.

Copyright © 2018 VMware, Inc. All Rights Reserved.

# C Specification
## Source Files
* rtpi.h
* pi_mutex.c
* pi_cond.c

## Packaged Collateral
* rtpi.h
* librtpi.a
* librtpi.so

## Types
### pi_mutex_t
Wrapper to pthread_mutex_t guranteed to be initialized using a
mutexattr with the PTHREAD_PRIO_INHERIT protocal set.

### pi_cond_t
New primitive modeled after the POSIX pthread_cond_t, with the following
modifications.

1. All wakeup events will wake the N highest priority waiters.
2. Waiters will be woken in priority FIFO order.
3. The associated mutex must be held when the condition variable is signaled or
broadcast.
4. The associated mutex must be passed as a parameter to the signal and
broadcast calls. The mutex is used to requeue woken waiters and avoid the
"thundering herd" effect.

## Functions
### PI Mutex
The PI Mutex API represents a subset of the Pthread Mutex API, written
specifically for priority inheritance aware condition variables. The calls wrap
a pthread_mutex_t internally, but it is not exposed to the caller to examine,
manipulate, or pass directly.

#### int pi_mutex_init(pi_mutex_t \*mutex, uint32_t flags)
Wrapper to pthread_mutex_init and pthread_mutexattr_setprotocol,
ensuring PTHREAD_PRIO_INHERIT is set, and allows for the specification
of process private or process shared.

The following attributes are not supported:
##### type:
* PTHREAD_MUTEX_RECURSIVE
* PTHREAD_MUTEX_ERRORCHECK
##### robust:
* PTHREAD_MUTEX_ROBUST
##### protocol:
* PTHREAD_PRIO_NONE
* PTHREAD_PRIO_PROTECT

##### Where flags are:
* RTPI_MUTEX_PSHARED
##### And future flags may include
* RTPI_MUTEX_ERRORCHECK
* RTPI_MUTEX_ROBUST

Returns 0 on success, otherwise an error number is returned.

#### int pi_mutex_destroy(pi_mutex_t \*mutex)
Simple wrapper to pthread_mutex_destroy.

#### int pi_mutex_lock(pi_mutex_t \*mutex)
Simple wrapper to pthread_mutex_lock.

#### int pi_mutex_trylock(pi_mutex_t \*mutex)
Simple wrapper to pthread_mutex_trylock.

#### int pi_mutex_unlock(pi_mutex_t \*mutex)
Simple wrapper to pthread_mutex_unlock.

### PI Condition
The PI Condition API represents a new implementation of a Non-POSIX PI aware
condition variable.

#### int pi_cond_init(pi_cond_t \*cond, uint32_t flags)

##### Where flags are:
* RTPI_COND_PSHARED

#### int pi_cond_destroy(pi_cond_t \*cond)

#### int pi_cond_wait(pi_cond_t \*cond, pi_mutex_t \*mutex)

#### int pi_cond_timedwait(pi_cond_t \*cond, pi_mutex_t \*mutex, const struct timespec \*restrict abstime)

#### int pi_cond_signal(pi_cond_t \*cond, pi_mutex_t \*mutex)

#### int pi_cond_broadcast(pi_cond_t \*cond, pi_mutex_t \*mutex)

## Initializers

#### DEFINE_PI_MUTEX(mutex, flags)

Defines and initializes a PI aware mutex.

#### DEFINE_PI_COND(condvar, flags)

Defines and initializes a PI aware conditional variable.

# C++ Specification

## Source files
* rtpi/mutex.hpp
* rtpi/condition_variable.hpp

## Types
### rtpi::mutex

Wrapper around the rtpi `pi_mutex_t` that is intended to work as a
replacement for [std::mutex](https://en.cppreference.com/w/cpp/thread/mutex).

### rtpi::condition_variable

Wrapper around the rtpi `pi_cond_t` that is intended to work mostly as a
drop-in replacement for [std::condition_variable](https://en.cppreference.com/w/cpp/thread/condition_variable).

Notable differences from `std::condition_variable`:
* `std::unique_lock<rtpi::mutex>` is used for the wait methods instead of `std::unique_lock<std::mutex>`
* `notify_one` and `notify_all` require a `std::unique_lock<rtpi::mutex>` parameter

# References
1. POSIX pthread API?
2. [Requeue-PI: Making Glibc Condvars PI-Aware](https://static.lwn.net/images/conf/rtlws11/papers/proc/p10.pdf)
3. [Bug 11588 - pthread condvars are not priority inheritance aware](https://sourceware.org/bugzilla/show_bug.cgi?id=11588)
