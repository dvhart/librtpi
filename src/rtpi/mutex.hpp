/* SPDX-License-Identifier: LGPL-2.1-only */

#ifndef RTPI_MUTEX_HPP
#define RTPI_MUTEX_HPP

#include <mutex>
#include <system_error>

#include "rtpi.h"

namespace rtpi
{
// The mutex class is a synchronization primitive that can be used to protect
// shared data from being simultaneously accessed by multiple threads.
//
// The API is based on the C++ std::mutex API.
//
// The mutex class satisfies the Mutex named requirement.

class mutex {
    private:
	pi_mutex m;

    public:
	typedef pi_mutex *native_handle_type;

	// Constructs the mutex. The mutex is in unlocked state after the constructor completes.
	constexpr mutex() noexcept : m(PI_MUTEX_INIT(0))
	{
	}

	// Copy constructor is deleted.
	mutex(const mutex &) = delete;

	// Destroys the mutex.
	~mutex()
	{
		pi_mutex_destroy(&m);
	}

	// Not copy-assignable.
	const mutex &operator=(const mutex &) = delete;

	// Locks the mutex. If another thread has already locked the mutex,
	// a call to lock will block execution until the lock is acquired.
	void lock()
	{
		int e = pi_mutex_lock(&m);

		if (e)
			throw std::system_error(
				std::error_code(e, std::generic_category()));
	}

	// Tries to lock the mutex. Returns immediately. On successful lock
	// acquisition returns true, otherwise returns false.
	bool try_lock()
	{
		// can return EBUSY or EDEADLOCK
		return !pi_mutex_trylock(&m);
	}

	// Unlocks the mutex.
	void unlock()
	{
		pi_mutex_unlock(&m);

		// pi_mutex_unlock might fail (EPERM, or errno from futex)
		// but the Mutex requirement states that unlock does not
		// throw exceptions.
	}

	// Returns the underlying implementation-defined native handle object.
	//
	// for librtpi, this is a pi_mutex*.
	native_handle_type native_handle()
	{
		return &m;
	}
};

} // namespace rtpi

#endif
