/* Copyright (C) 2002, 2010, 2013 Free Software Foundation, Inc.
   This file was submitted as a patch to the GNU C Library:
     https://sourceware.org/bugzilla/show_bug.cgi?id=11588
     https://sourceware.org/bugzilla/attachment.cgi?id=7689
   Contributed by Darren Hart <dvhltc@us.ibm.com>
   Based on pthread_cond_hang.c by Dinakar Guniguntala <dino@in.ibm.com>

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <error.h>
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "rtpi.h"

#include "rtpi/mutex.hpp"
#include "rtpi/condition_variable.hpp"

#define LOW_PRIO  1
#define MED_PRIO  2
#define HIGH_PRIO 3
#define MAIN_PRIO 4

static rtpi::condition_variable race_var;
static rtpi::mutex race_mut;

static rtpi::condition_variable sig1, sig2, sig3;
static rtpi::mutex m1, m2, m3;

static volatile unsigned int done = 0;

static void *
low_tf (void *p)
{
  int err;

  /* Wait for do_test to start all the threads.  */
  std::unique_lock<rtpi::mutex> m1_ul(m1);
  sig1.wait(m1_ul);

  std::unique_lock<rtpi::mutex> race_ul(race_mut);

  puts ("low_tf: locked");

  std::unique_lock<rtpi::mutex> m2_ul(m2, std::defer_lock);
  /* Signal the high_tf that we have the race_mut, it will preempt us until it
   * blocks on race_mut.  */
  sig2.notify_one(m2_ul);

  /* pi_cond_wait() holds the cond_lock when it unlocks race_mut.  high_tf
     will preempt us before we can release the cond_lock.  It will signal med_tf
     which will continue to block us after high_tf tries to block on race_var if
     it isn't PTHREAD_PRIO_INHERIT, and the cond_lock will never be released.  */
  race_var.wait(race_ul);

  puts ("low_tf: done waiting");
  race_ul.unlock();

  return NULL;
}

static void *
high_tf (void *p)
{
  int err;

  std::unique_lock<rtpi::mutex> m2_ul(m2);
  /* Wait for low_tf to take race_mut and signal us.  We will preempt low_tf
   * until we block on race_mut below.  */
  sig2.wait(m2_ul);

  /* Wait for low_tf to release the lock as it waits on race_var.  */
  std::unique_lock<rtpi::mutex> race_ul(race_mut);

  puts ("high_tf: locked");

  /* Signal the med_tf to start spinning.  */
  std::unique_lock<rtpi::mutex> m3_ul(m3, std::defer_lock);
  sig3.notify_one(m3_ul);

  /* If the race_var isn't PTHREAD_PRIO_INHERIT, we will block on the
     race_var cond_lock waiting for the low_tf to release it in it's
     pi_cond_wait(&race_var, &race_mut) call.  */
  race_var.wait(race_ul);

  puts ("high_tf: done waiting");

  race_ul.unlock();

  done = 1;
  return NULL;
}

static void *
med_tf (void *p)
{
  int err;

  std::unique_lock<rtpi::mutex> m3_ul(m3);
  /* Wait for high_tf to signal us.  */
  sig3.wait(m3_ul);

  puts ("med_tf: spinning");

  while (!done)
          /* Busy wait to block low threads.  */;

  puts ("med_tf: done spinning");

  return NULL;
}

static int
do_test (void)
{
  pthread_t low_thread;
  pthread_t med_thread;
  pthread_t high_thread;
  struct sched_param param;
  pthread_attr_t attr;
  cpu_set_t cset;
  int err;


  /* Initialize mutexes and condvars.  */

  err = pthread_attr_init (&attr);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to init pthread_attr");
  err = pthread_attr_setinheritsched (&attr, PTHREAD_EXPLICIT_SCHED);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set attr inheritsched");
  err = pthread_attr_setschedpolicy (&attr, SCHED_FIFO);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set attr schedpolicy");

  /* Setup scheduling parameters and create threads.  */
  param.sched_priority = MAIN_PRIO;
  err = sched_setscheduler (0, SCHED_FIFO, &param);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set scheduler policy");

  CPU_ZERO (&cset);
  CPU_SET (0, &cset);
  err = sched_setaffinity (0, sizeof (cpu_set_t), &cset);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set CPU affinity");

  param.sched_priority = LOW_PRIO;
  err = pthread_attr_setschedparam (&attr, &param);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set sched param for low_tf");
  err = pthread_create (&low_thread, &attr, low_tf, (void*)NULL);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to create low_tf");

  param.sched_priority = MED_PRIO;
  err = pthread_attr_setschedparam (&attr, &param);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set sched param for med_tf");
  pthread_create (&med_thread, &attr, med_tf, (void*)NULL);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to create med_tf");

  param.sched_priority = HIGH_PRIO;
  err = pthread_attr_setschedparam (&attr, &param);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set sched param for high_tf");
  err = pthread_create (&high_thread, &attr, high_tf, (void*)NULL);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to create high_tf");

  /* Wait for the threads to start and block on their respective condvars.  */
  usleep (1000);
  std::unique_lock<rtpi::mutex> m1_ul(m1, std::defer_lock);
  sig1.notify_one(m1_ul);

  /* Wake low_tf and high_tf, allowing them to complete.  If race_var is not
     PTHREAD_PRIO_INHERIT, low_tf will not have released the race_var cond_lock
     and neither thread will have waited on race_var.  */
  usleep (1000);
  std::unique_lock<rtpi::mutex> race_ul(race_mut, std::defer_lock);
  race_var.notify_all(race_ul);

  /* Wait for threads to complete.  */
  err = pthread_join (low_thread, (void**) NULL);
  if (err != 0)
    error (EXIT_FAILURE, err, "join of low_tf failed");
  err = pthread_join (med_thread, (void**) NULL);
  if (err != 0)
    error (EXIT_FAILURE, err, "join of med_tf failed");
  err = pthread_join (high_thread, (void**) NULL);
  if (err != 0)
    error (EXIT_FAILURE, err, "join of high_tf failed");

  puts ("done");

  return 0;
}

int main(void)
{
	do_test();
	return 0;
}
