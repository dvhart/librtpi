/* SPDX-License-Identifier: LGPL-2.1-only */

#ifndef RTPI_CONDITION_VARIABLE_HPP
#define RTPI_CONDITION_VARIABLE_HPP

#include <chrono>
#include <condition_variable>

#include "rtpi.h"
#include "rtpi/mutex.hpp"

namespace rtpi
{
// The condition_variable class is a synchronization primitive that can be
// used to block a thread, or multiple threads at the same time, until
// another thread both modifies a shared variable (the condition), and
// notifies the condition_variable.
//
// The API is based upon the C++ std::condition_variable API, with a
// notable difference in that notify_one/notify_all have an additional
// parameter for a std::unique_lock<rtpi::mutex>.
//
// rtpi::condition_variable works only with std::unique_lock<rtpi::mutex>.
//

using std::cv_status;

class condition_variable {
    private:
	pi_cond_t c;

    public:
	typedef pi_cond_t *native_handle_type;

	// Constructs the condition_variable. The condition_variable is in unlocked state after the constructor completes.
	constexpr condition_variable() noexcept : c(PI_COND_INIT(0))
	{
	}

	// Copy constructor is deleted.
	condition_variable(const condition_variable &) = delete;

	// Destroys the condition_variable.
	~condition_variable()
	{
		pi_cond_destroy(&c);
	}

	// Not copy-assignable.
	const condition_variable &
	operator=(const condition_variable &) = delete;

	// If any threads are waiting on *this, calling notify_one unblocks one of the waiting threads.
	void notify_one(std::unique_lock<rtpi::mutex> &lock) noexcept
	{
		pi_cond_signal(&c, lock.mutex()->native_handle());
	}

	// Unblocks all threads currently waiting for *this.
	void notify_all(std::unique_lock<rtpi::mutex> &lock) noexcept
	{
		pi_cond_broadcast(&c, lock.mutex()->native_handle());
	}

	// Atomically unlocks lock, blocks the current executing thread, and
	// adds it to the list of threads waiting on *this. The thread will be
	// unblocked when notify_all() or notify_one() is executed. It may
	// also be unblocked spuriously. When unblocked, regardless of the
	// reason, lock is reacquired and wait exits.
	void wait(std::unique_lock<rtpi::mutex> &lock)
	{
		int e = pi_cond_wait(&c, lock.mutex()->native_handle());

		if (e)
			std::terminate();
	}

	// Overload that takes a predicate. This overload may be used to
	// ignore spurious awakenings while waiting for a specific condition
	// to become true.
	template <class Predicate>
	void wait(std::unique_lock<rtpi::mutex> &lock, Predicate stop_waiting)
	{
		while (!stop_waiting()) {
			wait(lock);
		}
	}

	// Atomically releases lock, blocks the current executing thread, and
	// adds it to the list of threads waiting on *this. The thread will be
	// unblocked when notify_all() or notify_one() is executed, or when
	// the relative timeout rel_time expires. It may also be unblocked
	// spuriously. When unblocked, regardless of the reason, lock is
	// reacquired and wait_for() exits.
	template <class Clock, class Period>
	cv_status wait_for(std::unique_lock<rtpi::mutex> &lock,
			   const std::chrono::duration<Clock, Period> &rel_time)
	{
		using duration = std::chrono::steady_clock::duration;

		// When converting from a floating-point rel_time to the
		// (integer) steady_clock duration type, the loss of precision
		// may result in the time being rounded down, so round it
		// back up if necessary.
		auto relative_time =
			std::chrono::duration_cast<duration>(rel_time);
		if (relative_time < rel_time)
			++relative_time;

		return wait_until(lock, std::chrono::steady_clock::now() +
						relative_time);
	}

	// Overload that takes a predicate. This overload may be used to
	// ignore spurious awakenings while waiting for a specific condition
	// to become true.
	template <class Clock, class Period, class Predicate>
	bool wait_for(std::unique_lock<rtpi::mutex> &lock,
		      const std::chrono::duration<Clock, Period> &rel_time,
		      Predicate stop_waiting)
	{
		using duration = std::chrono::steady_clock::duration;

		// If the conversion requires it, round up.
		auto relative_time =
			std::chrono::duration_cast<duration>(rel_time);
		if (relative_time < rel_time)
			++relative_time;

		return wait_until(
			lock, std::chrono::steady_clock::now() + relative_time,
			std::move(stop_waiting));
	}

	// Atomically releases lock, blocks the current executing thread, and
	// adds it to the list of threads waiting on *this. The thread will be
	// unblocked when notify_all() or notify_one() is executed, or when
	// the absolute time point timeout_time is reached. It may also be
	// unblocked spuriously. When unblocked, regardless of the reason,
	// lock is reacquired and wait_until exits.
	template <class Duration>
	cv_status
	wait_until(std::unique_lock<rtpi::mutex> &lock,
		   const std::chrono::time_point<std::chrono::steady_clock,
						 Duration> &timeout_time)
	{
		return wait_until_impl(lock, timeout_time);
	}

	template <class Clock, class Duration>
	cv_status
	wait_until(std::unique_lock<rtpi::mutex> &lock,
		   const std::chrono::time_point<Clock, Duration> &timeout_time)
	{
		using std::chrono::steady_clock;

		const typename Clock::time_point user_clock_entry =
			Clock::now();
		const typename steady_clock::time_point steady_clock_entry =
			steady_clock::now();

		const auto delta = timeout_time - user_clock_entry;
		const auto steady_timeout_time = steady_clock_entry + delta;

		if (wait_until_impl(lock, steady_timeout_time) ==
		    cv_status::no_timeout) {
			return cv_status::no_timeout;
		}

		// We got a timeout against CLOCK_MONOTONIC but we need to check
		// against the caller-supplied clock.
		if (Clock::now() < timeout_time) {
			return cv_status::no_timeout;
		}

		return cv_status::timeout;
	}

	// Overload that takes a predicate. This overload may be used to
	// ignore spurious awakenings while waiting for a specific condition
	// to become true.
	template <class Clock, class Duration, class Predicate>
	bool
	wait_until(std::unique_lock<rtpi::mutex> &lock,
		   const std::chrono::time_point<Clock, Duration> &timeout_time,
		   Predicate stop_waiting)
	{
		while (!stop_waiting) {
			if (wait_until(lock, timeout_time) ==
			    cv_status::timeout)
				return stop_waiting();
		}

		return true;
	}

	// Returns the underlying implementation-defined native handle object.
	//
	// for librtpi, this is a pi_cond_t*.
	native_handle_type native_handle()
	{
		return &c;
	}

    private:
	template <class Duration>
	cv_status
	wait_until_impl(std::unique_lock<rtpi::mutex> &lock,
			const std::chrono::time_point<std::chrono::steady_clock,
						      Duration> &timeout_time)
	{
		auto s = std::chrono::time_point_cast<std::chrono::seconds>(
			timeout_time);
		auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
			timeout_time - s);

		struct timespec ts = { static_cast<std::time_t>(
					       s.time_since_epoch().count()),
				       static_cast<long>(ns.count()) };

		// pi_cond_timedwait uses CLOCK_MONOTONIC (steady_clock)
		int e = pi_cond_timedwait(&c, lock.mutex()->native_handle(),
					  &ts);

		if (e == 0) {
			return cv_status::no_timeout;
		} else if (e == ETIMEDOUT) {
			return cv_status::timeout;
		} else {
			throw std::system_error(
				std::error_code(e, std::generic_category()));
		}
	}
};

} // namespace rtpi

#endif
